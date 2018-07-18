#!/usr/bin/env python3

import cv2
import ellmanager as emanager
import io
import json
import logging
import model
import numpy as numpy
import os
import picamera
import random
import shutil
import subprocess
import sys
import termios
import time
import tty
import zipfile
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
    mp4_box = "MP4Box -fps {0} -quiet -add {1} {2}".format(capture_rate, input_path, output_path)
    run_shell(mp4_box)
    os.remove(input_path)
    os.rename(output_path, rename_path)
    logging.debug('Video Saved')


def model_predict(image):
    with open("categories.txt", "r") as cat_file:
        categories = cat_file.read().splitlines()

    input_shape = model.get_default_input_shape()
    input_data = emanager.prepare_image_for_model(image, input_shape.columns, input_shape.rows)
    prediction = model.predict(input_data)
    top_2 = emanager.get_top_n(prediction, 2)
    
    if (len(top_2) < 1):
        return None, None
    else:
        word = categories[top_2[0][0]]
        predict_value = top_2[0][1]
        return word, predict_value

def json_fill(video_time, word_prediction, predicition_value, video_name, json_path):
    json_messge = {
        'Description': {
            'sysTime':               str(datetime.now().isoformat()) + 'Z',
            'videoStartTime':        str(video_time.isoformat()) + 'Z',
            'prediction(s)':         word_prediction,
            'predictionConfidence':  str(predicition_value),
            'videoName':             video_name
        }
    }
    
    logging.debug("Rewriting Json to File")
    with open(json_path, 'w') as json_file:
        json.dump(json_messge, json_file)

def azure_download_from_path(model_container_name, model_dir_path, compressed_model_dir_path, compressed_model_name):
    #Download Azure Version to the Raspberry pi
        block_blob_service.get_blob_to_path(model_container_name, compressed_model_name, compressed_model_dir_path)
        os.makedirs(model_dir_path)
        zf = zipfile.ZipFile(compressed_model_dir_path)
        zf.extractall(model_dir_path)

def azure_model_update(update_json_path): 
    print('We are in the model_update function')
    
    # List the Models in the blob. There should only be one named zippedpi3
    model_blob_list = block_blob_service.list_blobs(model_container_name)
    for blob in model_blob_list:
        print(blob.name)
        if (blob.name == 'zippedpi3'):
            last_blob_update = blob.properties.last_modified
            print('Printing the date in UTC time format')
            print(last_blob_update)
        # Leave the loop once we found what we want
        break;
    
    # If we don't already got the json just go ahead and create a new one
    if not os.path.exists(update_json_path):
        dict = {'lastupdate': last_blob_update}
        holder = json.dump(dict)
        f.open(update_json_path, "w")
        f.write(j)
        f.close()

    # Now that we need know we have it, we need to parse it and decide if we need to update or not
    with open(update_json_path) as f:
        json_data = json.load(f)
    last_model_update = json_data["lastupdate"]


    # If the times are not equal and there has been a change somewhere Update
    if (last_model_update != last_blob_update):
        # Check to make sure that we have the pisetup.py script
        print ('They were not the same so I will be performing an update now')
        os.system('python3 pisetup.py')
        
        # Change the json file to represent the new modified time
        json_data["lastupdate"] = last_blob_update
        json.dump(json_data, f)



# Function to Upload a specified path to an object to Azure Blob Storage
def azure_upload_from_path(blob_container,blob_name,blob_object,blob_format):
    block_blob_service.create_blob_from_path(blob_container, blob_name,blob_object, content_settings=ContentSettings(content_type=blob_format))

def get_video():
    # Define Variables
    capture_time = 30
    capture_rate = 30.0
    preroll = 5
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
            camera_device.capture(image,'bgr', resize=camera_res, use_video_port=True)
            camera_device.wait_recording(1)
            
            # Take Picture for Azure
            image_name = "image-{0}.jpg".format(my_later.strftime("%Y%m%d%H%M%S"))
            image_path = "{0}/{1}".format(SCRIPT_DIR, image_name)
            print(image_path)
            camera_device.capture(image_path)
            camera_device.wait_recording(1)

            # Make Prediction with the first picture
            logging.debug('Prediction Captured')
            word, predict_value = model_predict(image)
            
            # Give time here for model predictions
            camera_device.wait_recording(3)
            logging.debug('Prediction Returned')
            my_now = datetime.now()
            
            if word is None:
                logging.debug('No Event Registered')
                capture_video = False
                # Format specifically for the Good Folder
                bad_image_folder = "{0}/badimages".format(picture_container_name)
                # Send Picture to the Bad Images Folder on Azure that can be used to retrain
                azure_upload_from_path(bad_image_folder, image_name, image_path, 'image/jpeg')
            elif word is not None and predict_value < 0.4:
                logging.debug('Prediction Value Too Low')
                capture_video = False
                # Format Specifically for the Good FOlder
                bad_image_folder = "{0}/badimages".format(picture_container_name)
                # Send Picture to the Bad Images Folder on Azure that can be used to retrain
                azure_upload_from_path(bad_image_folder, image_name, image_path, 'image/jpeg')
                camera_device.wait_recording(2)
            else:
                # See what we got back from the model
                logging.debug('Event Registered')
                capture_video=True
                print('Prediction(s): {}'.format(word))
                # Format specifically for the Good Folder
                good_image_folder = "{0}/goodimages".format(picture_container_name)
                # Send the Picture to the Good Images Folder on Azure
                azure_upload_from_path(good_image_folder, image_name, image_path, 'image/jpeg')
                camera_device.wait_recording(2)
                # Once it is uploaded, delete the image
                os.remove(image_path)
                break
            # If we don;t break by finidng the right predicition stay in the loop
            seconds_past = 0
            # Delete the image from the OS folder to save space
            os.remove(image_path)

    ## Create diretory to save the video that we get if we are told to capture video
    start_time = my_later
    base_dir = SCRIPT_DIR
    video_dir = "myvideos"
    video_dir_path ="{0}/{1}".format(base_dir, video_dir)

    if not os.path.exists(video_dir_path):
        os.makedirs(video_dir_path)

    video_start_time = start_time - timedelta(seconds=preroll)

    ## We will have two seperate files, one for before and after the event had been triggered
    #Before:
    before_event =         "video-{0}-{1}.h264".format("before", video_start_time.strftime("%Y%m%d%H%M%S"))
    before_event_path =    "{0}/{1}/{2}".format(base_dir, video_dir, before_event)
    before_mp4 =           before_event.replace('.h264', '.mp4')
    before_mp4_path =      "{0}/{1}/{2}".format(base_dir, video_dir, before_mp4)
    before_path_temp =      "{0}.tmp".format(before_mp4_path)

    # After:
    after_event =         "video-{0}-{1}.h264".format("after", video_start_time.strftime("%Y%m%d%H%M%S"))
    after_event_path =    "{0}/{1}/{2}".format(base_dir, video_dir, after_event)
    after_mp4 =           after_event.replace('.h264', '.mp4')
    after_mp4_path =      "{0}/{1}/{2}".format(base_dir, video_dir, after_mp4)
    after_path_temp =     "{0}.tmp".format(after_mp4_path)

    # Full combined video path
    full_path =           "video-{0}-{1}.mp4".format("full", video_start_time.strftime("%Y%m%d%H%M%S"))
    full_video_path =     "{0}/{1}/{2}".format(base_dir, video_dir, full_path)

    # Create a json file to a reference the given event
    json_file_name = "video-description-{0}.json".format(video_start_time.strftime("%Y%m%d%H%M%S"))
    json_file_path = "{0}/{1}/{2}".format(base_dir,video_dir, json_file_name)

    if capture_video == True:
        # Save the video to a file path specified
        camera_device.split_recording(after_event_path)
        video_stream.copy_to(before_event_path, seconds=preroll)
        camera_device.wait_recording(preroll+5)
                   
        # Convert to MP4 format for viewing
        save_video(capture_rate, before_event_path, before_path_temp, before_mp4_path)
        save_video(capture_rate, after_event_path, after_path_temp, after_mp4_path)

        # Upload Before Videos to Azure Blob Storage
        before_video_folder = "{0}/{1}".format(video_container_name, 'beforevideo')
        azure_upload_from_path(before_video_folder, before_mp4, before_mp4_path, 'video/mp4')

        # Upload After Videos to Azure Blob Storage
        after_video_folder = "{0}/{1}".format(video_container_name, 'aftervideo')
        azure_upload_from_path(after_video_folder, after_mp4, after_mp4_path, 'video/mp4')

        # Combine the two mp4 videos into one and save it
        full_video = "MP4Box -cat {0} -cat {1} -new {2}".format(before_mp4_path, after_mp4_path, full_video_path)
        run_shell(full_video)
        logging.debug('Combining Full Video')
        
        # Upload Video to Azure Blob Storage
        full_video_folder = "{0}/{1}".format(video_container_name, 'fullvideo')
        azure_upload_from_path(full_video_folder, full_path, full_video_path, 'video/mp4')

        # Create json and fill it with information
        json_fill(video_start_time, word, predict_value, full_path, json_file_path)

        # Upload Json to Azure Blob Storge
        azure_upload_from_path(json_container_name, json_file_name, json_file_path, 'application/json')
       
        # End Things
        shutil.rmtree(video_dir_path)
        camera_device.stop_recording()

    # Used to Delete Directory but it needs a time delat
    #shutil.rmtree(video_dir_path)

def main():
    # Define Globals
    global camera_device
    global block_blob_service

    # Maybe Make these none globals
    global picture_container_name
    global video_container_name
    global model_container_name
    global json_container_name
   
    #Define Varibles
    capture_rate = 30.0

    # Intialize Log Properties
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s') 

    # Intilize Camera properties 
    camera_device = picamera.PiCamera()
    camera_device.resolution = (1280, 720)
    camera_device.framerate = capture_rate
    
    if camera_device is None:
        logging.debug("No Camera Device Found.")
        sys.exit(1)

    # Intialize Azure Properties
    block_blob_service = BlockBlobService(account_name='**************', account_key='***************************')
   
    if block_blob_service is None:
        logging.debug("No Azure Storage Account Connected")
        sys.exit(1)

    # Create Neccesary Containers and Blobs if they don't exist already
    picture_container_name = 'edgeimages'
    video_container_name = 'edgevideos'
    model_container_name = 'edgemodels'
    json_container_name = 'edgejson'
    block_blob_service.create_container(picture_container_name)
    block_blob_service.create_container(video_container_name)
    block_blob_service.create_container(model_container_name)
    block_blob_service.create_container(json_container_name)
            
    # Intialize the updates Json File
    update_json_path = "{0}/{1}.json".format(SCRIPT_DIR, 'updatehistory')
           
    # Constantly run the Edge.py Script
    while True:
        logging.debug('Starting Edge.py')

        # Check and Run Model Updates
        azure_model_update(update_json_path)
            
        # Began running and stay running the entire project.
        get_video()

if __name__ == '__main__':
    main()

                   
                   
        	    


            

