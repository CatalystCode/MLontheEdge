#!/usr/bin/env python3

## Import Necessary Python Libraries
import io
import random
import subprocess
import picamera
import sys
import termios
import tty
import os
import time
from PIL import Image
from datetime import datetime, timedelta

#Define Variables
global camera_device
global video_stream
global capture_time
global preroll
global capture_rate


## Get Working Directory
SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

## Get Character input from keyboard press
def getch():
    fd = sys.stdin.fileno()
    oldS = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, oldS)
    return ch

## Function for running specific shell commands
def run_shell(cmd):
    """
    Used for running shell commands
    """
    output = subprocess.check_output(cmd.split(' '))
    return str(output.rstrip().decode())

def start_camera():
    ##Define Variables
    capture_rate = 30 #FPS
    capture_time = 30 #Seconds
    with picamera.PiCamera() as camera_device:
        camera_device.start_preview()
        try:
            camera_device.resolution = (1280, 720)
            camera_device.framerate = capture_rate
            video_stream = picamera.PiCameraCircularIO(camera_device, capture_time)
            camera_device.start_recording(video_stream, format='h264')
        except Exception as ex:
            camera_device = None
            video_stream = None

def stop_camera():
    if(camera_device is not None):
        try:
            camera_device.close()
            camera_evice = None
            if (video_stream is not None):
                video_stream.close()
                video_stream = None
        except Exception as ex:
            # print("Camera and Buffer failed to close") make a log statement
            camera_device = None
            video_stream = None

def get_video():
    ## Start the camera and begin recording for video
    start_camera()

    if camera_device is None:
        ## Print Log Error
        return

    get_key = True
    capture_video = False
    buttonDelay = 0.2 #Seconds
    preroll = 7 # Seconds to go back before an event
    
    ## Create a folder to save the videos
    start_time = datetime.now()
    base_dir = SCRIPT_DIR
    video_dir = "myvideos"
    video_dir_path = "{0}/{1}".format(base_dir,video_dir)

    if not os.path.exists(video_dir_path):
        os.makedirs(video_dir_path)
    
    video_start_time =     start_time - timedelta(seconds=preroll)
    ## Set up seperate paths for before and after the event
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
    try:
        while (get_key == True):
            char = getch()
            camera_device.wait_recording(1)

            if (char == "p"):
            ##Change to log debiugs
            exit(0)
        
            if (char == "s"):
                capture_video = True
                time.sleep(button_delay)
                get_key == False
            else
                get_key == True
    except Exception as ex:
            camera_device = None
            video_stream = None

    if (capture_video == True):
        camera_device.split_recording(after_event_path) #We know it is an event so continue recording
        video_stream.copy_to(before_event_path, seconds = preroll)
        camera_device.wait_recording(20) #Continue recording to the after path for 20 seconds
        
        mp4box_before = "MP4Box -fps {0} -quiet -add {1} {2}".format(captureRate,before_event_path,before_path_temp) 
        mp4box_after  = "MP4Box -fps {0} -quiet -add {1} {2}".format(captureRate,after_event_path,after_path_temp)

        run_shell(mp4box_before)
        run_shell(mp4box_after)
        os.remove(before_event_path)
        os.remove(after_event_path)
        
def main():
    get_video()



if __name__ == '__main__':
    main()
