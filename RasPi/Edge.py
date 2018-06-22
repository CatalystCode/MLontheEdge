#!/usr/bin/env python3


import os
import random
import subprocess
import sys
import io
import termios
import tty
import time 
import picamera
import cv2
import ellmanager as emanager
import model
import numpy as numpy
from datetime import datetime, timedelta

SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

def run_shell(cmd):
    """
    Used for running shell commands
    """
    output = subprocess.check_output(cmd.split(' '))
    return str(output.rstrip().decode())

def getch():
    """
    Used to get a keyboard input from the user
    """
    fd = sys.stdin.fileno()
    oldS = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, oldS)
    return ch

def save_video(capture_rate,input_path,output_path,rename_path):
    ## Convert each indivudul .h264 to mp4 
    mp4_box = "MP4Box -fps {0} -quiet -add {1} {2}".format(capture_rate,input_path,output_path)
    run_shell(mp4_box)
    os.remove(input_path)
    os.rename(output_path,rename_path)


def model_predict(image):
    with open("categories.txt", "r") as cat_file:
        categories = cat_file.read().splitlines()

    input_shape = model.get_default_input_shape()
    input_data = emanager.prepare_image_for_model(image, input_shape.columns, input_shape.rows)
    print("Image worked")
    prediction = model.predict(input_data)
    print("Prediction Worked")
    top_5 = emanager.get_top_n(prediction, 5)
    print("Top 5 worked")
    print(top_5)
    print("Below is the prediction")
    print(categories[top_5[0][0]])
    #print(categories[top_5[0]])
    
    #Here we would print the word and return it back to the code below for work
#
#
#    print("We are going to make a prediction here")

def get_video():
    ## Define Variables
    capture_time = 30
    capture_rate = 30.0
    preroll = 10
    get_key = True
    capture_video = False
    
    camera_res = (256,256)
    image = numpy.empty((camera_res[1], camera_res[0],3), dtype=numpy.uint8)

    video_stream = picamera.PiCameraCircularIO(camera_device, seconds=capture_time)
    camera_device.start_preview()
    camera_device.start_recording(video_stream, format='h264')
    my_now = datetime.now()

    while True:
        my_later = datetime.now()
        difference = my_later-my_now
        camera_device.wait_recording(1)
        if difference.seconds > preroll+1:
            capture_video = True
            print("Event Happened")
            
            camera_device.capture(image,'bgr',resize=camera_res,use_video_port=True)
            camera_device.wait_recording(2)
            model_predict(image)
            break
        else:
            capture_video = False

    
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

    #After:
    after_event =         "video-{0}-{1}.h264".format("after",video_start_time.strftime("%Y%m%d%H%M%S"))
    after_event_path =    "{0}/{1}/{2}".format(base_dir,video_dir, after_event)
    after_mp4 =           after_event.replace('.h264','.mp4')
    after_mp4_path =      "{0}/{1}/{2}".format(base_dir,video_dir,after_mp4)
    after_path_temp =     "{0}.tmp".format(after_mp4_path)

    #Full combined video path
    full_path =           "video-{0}-{1}.mp4".format("full",video_start_time.strftime("%Y%m%d%H%M%S"))
    full_video_path =     "{0}/{1}/{2}".format(base_dir,video_dir,full_path)

    if capture_video == True:
        ##Save the video to a file path specified
        camera_device.split_recording(after_event_path)
        video_stream.copy_to(before_event_path, seconds=preroll)
        camera_device.wait_recording(preroll+5)
                   
        #Convert to MP4 format for viewing
        save_video(capture_rate,before_event_path,before_path_temp,before_mp4_path)
        save_video(capture_rate,after_event_path,after_path_temp,after_mp4_path) 
        
        #Combine the two mp4 videos into one and save it
        full_video = "MP4Box -cat {0} -cat {1} -new {2}".format(before_mp4_path, after_mp4_path, full_video_path)
        run_shell(full_video)
        
        camera_device.stop_recording()

def main():
    #Define Variables
    global camera_device
    capture_rate = 30.0
    
    ## Set the camera properties up
    camera_device = picamera.PiCamera()
    camera_device.resolution = (1280, 720)
    camera_device.framerate = capture_rate
    while True:
        print("Starting Get Video")
        get_video()

if __name__ == '__main__':
    main()

                   
                   
        	    


            

