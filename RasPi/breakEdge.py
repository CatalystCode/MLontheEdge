#!/usr/bin/env python3


import os
import random
import subprocess
import sys
import io
import termios
import tty
import time
import logging
import picamera
import cv2
import ellmanager as emanager
import model
import numpy as numpy
from datetime import datetime, timedelta
from azure.storage.blob import BlockBlobService, ContentSettings, PublicAccess

SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

def run_shell(cmd):
    """
    Used for running shell commands
    """
    output = subprocess.check_output(cmd.split(' '))
    logging.debug('Running shell command')
    return str(output.rstrip().decode())

def save_video(capture_rate,input_path,output_path,rename_path):
    # Convert each indivudul .h264 to mp4 
    mp4_box = "MP4Box -fps {0} -quiet -add {1} {2}".format(capture_rate,input_path,output_path)
    run_shell(mp4_box)
    os.remove(input_path)
    os.rename(output_path,rename_path)
    logging.debug('Video Saved')


def model_predict(image):
    with open("categories.txt", "r") as cat_file:
        categories = cat_file.read().splitlines()

    input_shape = model.get_default_input_shape()
    input_data = emanager.prepare_image_for_model(image, input_shape.columns, input_shape.rows)
    prediction = model.predict(input_data)
    top_5 = emanager.get_top_n(prediction, 5)
    
    if (len(top_5) < 1):
        return None
    else:
        word = categories[top_5[0][0]]
        return word

def get_video():
    # Define Variables
    capture_time = 30
    capture_rate = 30.0
    preroll = 10
    get_key = True
    capture_video = False
    camera_res = (256,256)
    image = numpy.empty((camera_res[1], camera_res[0],3), dtype=numpy.uint8)

    # Set up Circular Buffer Settings
    video_stream = picamera.PiCameraCircularIO(camera_device, seconds=capture_time)
    camera_device.start_preview()
    camera_device.start_recording(video_stream, format='h264')
    my_now = datetime.now()

    while True:
        # Set up a waiting time difference
        my_later = datetime.now()
        difference = my_later-my_now
        seconds_past = difference.seconds
        camera_device.wait_recording(1)

        logging.debug('Analyzing Surroundings')
        if seconds_past > preroll+1:
            # Take Picture for the Model
            camera_device.capture(image,'bgr',resize=camera_res,use_video_port=True)
            camera_device.wait_recording(1)
            
            # Take Picture for Azure
            image_path = "{0}/image-{1}".format(SCRIPT_DIR,my_later.strftime("%Y%m%d%H%M%S"))
            camera_device.capture(image_path)
            camera_device.wait_recoding(1)

            # Make Prediction with the first picture
            logging.debug('Prediction Captured')
            word_predict = model_predict(image)
            logging.debug('Prediction Returned')
            
            # See what we got back from the model
            if word_predict is not None:
                logging.debug('Event Registered')
                capture_video=True
                print('Prediction(s): {}'.format(word_predict))

                # Send the Picture to the Good Images Folder on Azure
                # DELETE THIS LINE: Call a function here that automatically uploads to azure

                break
            else:
                logging.debug('No Event Registered')
                my_now = datetime.now()
                capture_video=False

                # Send Picture to the Bad Images Folder on Azure that can be used to retrain
                # DELETE THIS LINE: Call a function here that automatically uploads to azure

            seconds_past = 0
            # Delete the image from the OS folder to save space
    
    ## Create diretory to save the video that we get if we are told to capture video
    start_time = datetime.now()
    base_dir = SCRIPT_DIR
    video_dir = "myvideos"
    video_dir_path ="{0}/{1}".format(base_dir, video_dir)

    if not os.path.exists(video_dir_path):
        os.makedirs(video_dir_path)

    video_start_time = start_time - timedelta(seconds=preroll)

    ## We will have two seperate files, one for before and after the event had been triggered
    #Before:
    before_event =         "video-{0}-{1}.h264".format("before",video_start_time.strftime("%Y%m%d%H%M%S"))
    before_event_path =    "{0}/{1}/{2}".format(base_dir,video_dir,before_event)
    before_mp4 =           before_event.replace('.h264','.mp4')
    before_mp4_path =      "{0}/{1}/{2}".format(base_dir,video_dir,before_mp4)
    before_path_temp =      "{0}.tmp".format(before_mp4_path)

    # After:
    after_event =         "video-{0}-{1}.h264".format("after",video_start_time.strftime("%Y%m%d%H%M%S"))
    after_event_path =    "{0}/{1}/{2}".format(base_dir,video_dir, after_event)
    after_mp4 =           after_event.replace('.h264','.mp4')
    after_mp4_path =      "{0}/{1}/{2}".format(base_dir,video_dir,after_mp4)
    after_path_temp =     "{0}.tmp".format(after_mp4_path)

    # Full combined video path
    full_path =           "video-{0}-{1}.mp4".format("full",video_start_time.strftime("%Y%m%d%H%M%S"))
    full_video_path =     "{0}/{1}/{2}".format(base_dir,video_dir,full_path)

    if capture_video == True:
        # Save the video to a file path specified
        camera_device.split_recording(after_event_path)
        video_stream.copy_to(before_event_path, seconds=preroll)
        camera_device.wait_recording(preroll+5)
                   
        # Convert to MP4 format for viewing
        save_video(capture_rate,before_event_path,before_path_temp,before_mp4_path)
        save_video(capture_rate,after_event_path,after_path_temp,after_mp4_path)

        # DELETE THIS LINE: Call a function here that automatically uploads to azure: Before

        # DELETE THIS LINE: Call a function here that automatically uploads to azure: After

 
        
        # Combine the two mp4 videos into one and save it
        full_video = "MP4Box -cat {0} -cat {1} -new {2}".format(before_mp4_path, after_mp4_path, full_video_path)
        run_shell(full_video)
        logging.debug('Combining Full Video')
        
        # DELETE THIS LINE: Call a function here that automatically uploads to azure
        block_blob_service.create_blob_from_path('images', full_path, full_video_path, content_settings=ContentSettings(content_type='video/mp4'))
        camera_device.stop_recording()

def main():
    # Define Variables
    global camera_device
    capture_rate = 30.0
   
    # Intialize Azure Properties
    global block_blob_service
    block_blob_service = BlockBlobService(account_name='*****', account_key='************************')
    
    # DELETE THIS LINE: Call a function here that automatically checks the model blob and downlods from azure

    # Intialize Log Properties
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s') 

    # Intilize Camera properties 
    camera_device = picamera.PiCamera()
    camera_device.resolution = (1280, 720)
    camera_device.framerate = capture_rate

    # Constantly run the Edge.py Script
    while True:
        logging.debug('Starting Edge.py')
        get_video()

if __name__ == '__main__':
    main()

                   
                   
        	    


            

