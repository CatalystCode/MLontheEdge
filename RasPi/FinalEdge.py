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

def get_video():
    ## Define Variables
    capture_time = 10
    preroll = 7
    capture_rate = 30.0
    get_key = True
    capture_video = False
    
    ## Set the camera properties up
    camera_device.resolution = (1280, 720)
    camera_device.framerate = capture_rate
    video_stream = picamera.PiCameraCircularIO(camera_device, seconds=30)
    camera_device.start_preview()
    camera_device.start_recording(video_stream, format='h264')
    
    ## Actually get the key from the keyboard to be used as an event
    try:
        while get_key: 
            char = getch()
            camera_device.wait_recording(1)
                
            if (char == "e"):
                capture_video = False
                get_key = False
                break
                
            if (char == "p"):
                capture_video = True
                get_key = False
                break
            else:
                get_key = True
    finally:
           print("In finally")

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

    if capture_video == True:
    ##Save the video to a file path specified
        camera_device.split_recording(after_event_path)
        video_stream.copy_to(before_event_path, seconds=5)
        camera_device.wait_recording(5)
                   
        #Convert to MP4 format for viewing
        mp4box_before = "MP4Box -fps {0} -quiet -add {1} {2}".format(capture_rate,before_event_path,before_path_temp) 
        mp4box_after  = "MP4Box -fps {0} -quiet -add {1} {2}".format(capture_rate,after_event_path,after_path_temp)

        run_shell(mp4box_before)
        run_shell(mp4box_after)
        os.remove(before_event_path)
        os.remove(after_event_path)
             
        camera_device.stop_recording()
def main():
    global camera_device
    camera_device = picamera.PiCamera()
    
    while True:
        print("Starting Get Video")
        get_video()

if __name__ == '__main__':
    main()

                   
                   
        	    


            

