# MLontheEdge
The overall purpose of this document is showcase example of Azure Machine Learning on IoT Edge Devices using Microsoft Embedded Learning Library (ELL)

## Table of Contents
1. [Windows Device Set Up](https://github.com/CatalystCode/MLontheEdge/tree/dev#window-device-set-up)
* [ELL for Windows Devices](https://github.com/Microsoft/ELL/blob/master/INSTALL-Windows.md) 
2. [Set Up for Raspberry Pi Devices](https://github.com/CatalystCode/MLontheEdge/tree/dev#set-up-for-raspberry-pi-devices)
* Python 3.5.3
* Change Hostname
* Camera Set up
* Enable SSH
3. [Programming Tools](https://github.com/CatalystCode/MLontheEdge/tree/dev#programming-tools)
3. [Download PreTrained Model](https://github.com/CatalystCode/MLontheEdge/tree/dev#download-models)
4. [Running Application](https://github.com/CatalystCode/MLontheEdge/tree/dev#running-applications)

## Window Device Set Up
The first step is to install Microsoft ELL on your host device. In order to do so, simply follow the directions in the link provided: [Microsoft ELL](https://github.com/Microsoft/ELL/blob/master/INSTALL-Windows.md)

## Set Up for Raspberry Pi Devices
1. **Python 3.5.3:** These steps assume you are starting from an existing Raspbian install with Python 3.5.3 on it based on the [Raspbian Stretch image - 2018-04-18](http://downloads.raspberrypi.org/raspbian/images/raspbian-2018-04-19/2018-04-18-raspbian-stretch.zip) image

2. **Change Hostname:** It is important that your host name is changes as we will attempt to connect to it remotely. The steps on how to achieve just that is best described here: [Hostname Directions](https://www.dexterindustries.com/howto/change-the-hostname-of-your-pi/)

3. **Camera Set Up:**
Begin by typing: 
```bash
sudo raspi-config
```
1. Select **5 Interfacing Options** and press **Enter**.
2. Select **P1 Camera** and press **Enter**.
3. Select **Yes** to enable the camera interface.
4. Load the camera module:
```bash
sudo modprobe bcm2835-v4l2
```
Once the picamera is plugged in, a simple test to see if you picamera is working correctly is the command ***raspistill:***
```bash
raspistill -o image.jpg
```
If a preview window is opened and a new file **image.jpg** is saved, then the picamera is successfully installed.

4. **Enable SSH:**
Begin by typing: 
```bash
sudo raspi-config
```
1. Select **5 Interfacing Options** and press **Enter**.
2. Select **P2 SSH** and press **Enter**
3. Select **Yes** to enable the SSH Server.
4. ***Note:*** : Once SSH has been enabled, be sure to change the password associated with your Raspberry Pi, if you have not done so already.

5. **Network Connection:**
For this project, the majority of the network connectivity came through the attachment of an ethernet cable. However, attached are steps to connecting the Raspberry Pi to a wireless connection. [Wifi Connections](https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md)

## **Programming Tools:**

**CMake:**
CMake will be used on the Raspberry Pi to create python modules that can be called from given code. In order to install CMake on your Raspberry Pi, you must first be connected to the network, then open a terminal window and type:
```bash
sudo apt-get update
sudo apt-get install -y cmake
```
**OpenBLAS:**
This is for fast linear algerba operations. This is highly recommended because it can significantly increase the speed of the models. Type:
```bash
sudo apt-get install -y libopenblas-dev
```

**Curl:**
Curl is a command line tool used to transfer Data via URL. To install ***curl,*** type: 
```bash
sudo apt-get install -y curl
```

**OpenCV:** As of this time, you will need to Build Open CV 3.3.0 on Raspbian Stretch with Python 3.5.3
1. It is crucial that this is done on Raspbian Stretch with Python 3.5.3. A link to download and format for Raspbian Stretch is provided here. [Strech Image](https://www.raspberrypi.org/documentation/installation/installing-images/)
2. Make sure that the Raspbian Stretch OS is up to date:
```bash
sudo apt-get update
sudo apt-get upgrade -y
```
3. Ensure that after the update, Python 3 is running **Python 3.5.3**:
```bash
python3 --version
```
4. Install some pre-requisities for the OpenCV Download:
```bash
sudo apt-get install -y build-essential cmake pkg-config
sudo apt-get install -y libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev libdc1394-22-dev
sudo apt-get install -y libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
sudo apt-get install -y libxvidcore-dev libx264-dev
sudo apt-get install -y libgtk2.0-dev libgtk-3-dev
sudo apt-get install -y libatlas-base-dev gfortran
sudo apt-get install -y python3-dev
```
5. Ensure that after the install, we running the correct pip module version:
```bash
python3 -m pip --version
```
Verify that the result of that command is a pip version 9.0.1 or greater
```bash
pip 9.0.1 from /usr/lib/python3/dist-packages (python 3.5)
```
6. Install Jinja2:
```bash
python3 -m install jinja2
```
7. Install Numpy:
```bash
python3 -m pip install numpy
```
8. Download and Unzip the OpenCV Source:
```bash
cd ~
wget -O opencv.zip 'https://github.com/Itseez/opencv/archive/3.3.0.zip'
unzip opencv.zip
```
Switch into folder and download the Opencv_contrib repo:
```bash
cd opencv-3.3.0
wget -O opencv_contrib.zip 'https://github.com/Itseez/opencv_contrib/archive/3.3.0.zip'
unzip opencv_contrib.zip
```
9. Create and go into the following **build** directory
```bash
cd ~/opencv-3.3.0
mkdir build
cd build
```
10. Run cmake with the following commands and **DON'T FORGET THE '..' AT THE END** :
```bash
cmake -D CMAKE_BUILD_TYPE=RELEASE \
  -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib-3.3.0/modules \
  -D BUILD_EXAMPLES=OFF \
  -D BUILD_TESTS=OFF \
  -D BUILD_PERF_TESTS=OFF \
  -D BUILD_opencv_python2=0 \
  -D PYTHON2_EXECUTABLE= \
  -D PYTHON2_INCLUDE_DIR= \
  -D PYTHON2_LIBRARY= \
  -D PYTHON2_NUMPY_INCLUDE_DIRS= \
  -D PYTHON2_PACKAGES_PATH= \
  -D BUILD_opencv_python3=1 \
  -D PYTHON3_EXECUTABLE=/usr/bin/python3 \
  -D PYTHON3_INCLUDE_DIR=/usr/include/python3.5 \
  -D PYTHON3_LIBRARY=/usr/lib/arm-linux-gnueabihf/libpython3.5m.so \
  -D PYTHON3_NUMPY_INCLUDE_DIRS=/home/pi/.local/lib/python3.5/site-packages/numpy/core/include \
  -D PYTHON3_PACKAGES_PATH=/home/pi/.local/lib/python3.5/site-packages \
  ..
```
11. Once that is complete, Run **make:**

***Note:*** **make** will take approximately 2 hours to complete.
```bash
make
```
12. Run **make install:**
```bash
sudo make install
```
13. Test that OpenCV was installed correctly:
```bash
python3 -c "import cv2; print(cv2.__version__)"
```
14. Remove the unneccesay zip file and directory
```bash
rm ~/opencv.zip
rm -rf ~/opencv-3.3.0
```


## Download Models
As of right now, the Microsoft ELL supports Neural Network Models that were trained with Microsoft Cognitive Toolkit(CNTK) or with Darknet. Follow the given tutorial for insights in how to download model with the ELL Library. [Importing Models.](https://microsoft.github.io/ELL/tutorials/Importing-models/)

## Running Applications
The steps below show how the given Azure ML on Edge Project is deployed
1. Clone or Download the given repository:
```
git clone https://github.com/CatalystCode/MLontheEdge.git
```
2. Switch in the MLontheEdge and Raspi Folders
```bash
cd MLontheEdge/Raspi/
```
3. Run the Edge.py script adapted specially for the Raspberry Pi
```python
python3 Edge.py
```
4. While the script is running, a camera preview window will be opened allow you to see what the picamera sees. The scripts takes a picture every 5 seconds and returns what the model thinks it sees in that picture.
5. If the model and python script recognize an object, a video is captured of the 10 seconds before that moment and 15 seconds after the given moment and then saved in a new directory **myvideos.** 
```bash
cd myvideos\
```
6. The video is in .mp4 format and can be viewed with any MP4 video player or even from the command line:
```bash
omxplayer "filename"
```