#!/usr/bin/env python3
###############################################################################
#
#  Project:  AzureML on the Edge
#  File:     onEdge.py
#  Authors:  David (Seun) Odun-Ayo
#
#  Requires: Python 3.x
#
###############################################################################

import os
import random
import subprocess
import sys
import io
import time
import picamera
from datetime import datetime, timedelta

######## Missing Pieces ##########
##import model:  Need the model created pi3 folder
##import cv2/pil: Used to preprocess the image. Transition from CV2 to Pil
##import ellmanager as manager: Waiting on cv2/pil

###### Set up variables ###
cameraDevice = None
videoStream = None

### Working Directory
SCRIPT_DIR = os.path.split(os.path.realpath(__file__))[0]

def run_shell(cmd):
    """
    Used for running shell commands
    """
    output = subprocess.check_output(cmd.split(' '))
    return str(output.rstrip().decode())

def start_camera():
    global cameraDevice
    global videoStream
    
    ## Will need to decided the finl resting place for these variables
    global captureTime 
    captureTime = 20
    #This might need to be negative
    global preroll
    preroll = 3
    ### HUB Manager This
    global captureRate 
    captureRate = 30.0
    with picamera.PiCamera() as cameraDevice:
        #Add a check for cameraDevice here
        cameraDevice.start_preview()
        try:
            #This can be set from a Hub Manager
            cameraDevice.resolution = (1280, 720) #This needs to be a set to a resolution that the model can understand: I think (256,256)
            cameraDevice.framerate = captureRate
            videoStream = picamera.PiCameraCircularIO(cameraDevice, seconds=captureTime + preroll)
            cameraDevice.start_recording(videoStream, format='h264')
            time.sleep(captureTime)
            #cameraDevice.stop_preview()
        except Exception as ex:
            print("The Buffer or Camera could not be intialized")
            cameraDevice = None
            videoStream = None
        finally:
            cameraDevice.stop_preview()

def stop_camera():
    global cameraDevice, videoStream
    if(cameraDevice is not None):
        try:
            cameraDevice.close()
            cameraDevice = None
            if (videoStream is not None):
                videoStream.close()
                videoStream = None
        except Exception as ex:
            print("Camera and Buffer failed to close")
            cameraDevice = None
            videoStream = None

def get_video():
    global recording, cameraDevice, videoStream
    print("In Get video bout to go to start_camera")
    start_camera()
    print("We worked with no issue in start camera")

    if cameraDevice is None:
        print("There is no camera device")
        return
  
    captureVideo = True ## This changed based on the model
    preroll = 3
    ##Create the folder for the videos
    startTime = datetime.now()
    baseDir = SCRIPT_DIR
    videoDir = "myvideos"
    videoDirPath = "{0}/{1}".format(baseDir,videoDir)
      
    if not os.path.exists(videoDirPath):
        os.makedirs(videoDirPath)

    videoStartTime =     startTime - timedelta(seconds=preroll)
    h264FileName =       "video-{0}-{1}.h264".format("seun",videoStartTime.strftime("%Y%m%d%H%M%S"))
    h264FilePath =       "{0}/{1}/{2}".format(baseDir,videoDir,h264FileName)
    mp4FileName =        h264FileName.replace('.h264','.mp4')
    mp4FilePath =        "{0}/{1}/{2}".format(baseDir,videoDir,mp4FileName)
    mp4FilePathTemp =    "{0}.tmp".format(mp4FilePath)
    mp4BlobName =        "{0}/{1}".format(videoDir,mp4FileName)
    jsonFileName =       "{0}.json".format(mp4FileName)
    jsonFilePath =       "{0}/{1}/{2}".format(baseDir,videoDir,jsonFileName)

    ##Save the video to a file path specified
    videoStream.copy_to(h264FilePath, seconds=captureTime + preroll)

    #Convert to MP4 format for viewing
    mp4box = "MP4Box -fps {0} -quiet -add {1} {2}".format(captureRate,h264FilePath,mp4FilePathTemp) 
    run_shell(mp4box)
    os.rename(mp4FilePathTemp, mp4FilePath)


def main():
    print("Welcome to main") 
    get_video()


if __name__ == '__main__':
    main()
