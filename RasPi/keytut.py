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


camera = picamera.PiCamera()
stream = picamera.PiCameraCircularIO(camera, seconds=20)
camera.start_recording(stream, format='h264')
camera.start_preview()
bDelay = 0.2
try:
    while True:
        char = getch()
        camera.wait_recording(1)

        if (char == "p"):
            print("Stop")
            exit(0)
        
        if (char == "s"):
            #camera.wait_recording(10)
            #stream.copy_to('video.h264')
            #time.sleep(bDelay)
            camera.split_recording('after.h264')
            stream.copy_to('before.h264', seconds = 10)
            camera.wait_recording(10)
            exit(0)
        elif (char == "a"):
            print("letter a was pressed")
            time.sleep(bDelay)
finally:
    print("I am in finally")
    camera.stop_recording()

