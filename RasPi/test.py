#!/usr/bin/env python3

import cv2
import io
import json
import logging
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
# Import the right folder and file
#import ellmanager as emanager
#import model

def main():
    for i in range(10):    
        print('This is a test that worked')

    print('Camera is about to start')
    camera = picamera.PiCamera()
    camera.resolution = (640, 480)
    camera.start_recording('testvideo.h264')
    camera.wait_recording(10)
    camera.stop_recording()
    print('Camera Worked and stoped recording')


if __name__ == '__main__':
    main()
