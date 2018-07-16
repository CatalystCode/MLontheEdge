# MLontheEdge
The overall purpose of this document is to showcase an example of Azure Machine Learning on IoT Edge Devices using Microsoft Embedded Learning Library (ELL)

## Table of Contents
1. [Acquiring Equipments](https://github.com/CatalystCode/MLontheEdge#acquiring-equipments)
2. [Windows Device Set Up](https://github.com/CatalystCode/MLontheEdge#window-device-set-up)
* [ELL for Windows Devices](https://github.com/Microsoft/ELL/blob/master/INSTALL-Windows.md) 
3. [Set Up for Raspberry Pi Devices](https://github.com/CatalystCode/MLontheEdge#set-up-for-raspberry-pi-devices)
4. [Programming Tools](https://github.com/CatalystCode/MLontheEdge#programming-tools)
5. [Download PreTrained Model](https://github.com/CatalystCode/MLontheEdge#download-models)
6. [Running Application](https://github.com/CatalystCode/MLontheEdge#running-applications)

## Acquiring Equipments
<table>
  <tr>
    <th>Item</th>
    <th>Description</th>
    <th style = "text-align:right">Est. Cost (USD)</th>
    <th>Purchase Link</th>
  </tr>
    <tr>
    <td>Raspberry Pi 3</td>
    <td>Main device for running software</td>
    <td style="text-align:right">$37.00</td>
    <td><a target="_blank" href="https://ebay.to/2KlncsD">Purchase</a></td>
  </tr>
  <tr>
    <td>Sandisk Ultra 32GB Micro SDHC</td>
    <td>Will store the Raspbian Operating System image and software for the Raspberry Pi 3</td>
    <td style="text-align:right">$12.00</td>
    <td><a target="_blank" href="http://a.co/eCokiTM">Purchase</a></td>
  </tr>
  <tr>
    <td>Raspberry Pi Camera</td>
    <td>Used to capture images and video on the Pi</td>
    <td style="text-align:right">$6.00</td>
    <td><a target="_blank" href="https://ebay.to/2Kv1ezf">Purchase</a></td>
  </tr>
  <tr>
    <td>USB Keyboard</td>
    <td>WIRED CONNECTION: Keyboard control for the Pi</td>
    <td style="text-align:right">$15.00</td>
    <td><a target="_blank" href="https://ebay.to/2IGqv7U">Purchase</a></td>
  </tr>
  <tr>
    <td>USB Mouse</td>
    <td>WIRED CONNECTION: Mouse control for the Pi</td>
    <td style="text-align:right">$6.00</td>
    <td><a target="_blank" href="https://ebay.to/2Kmdsy4">Purchase</a></td>
  </tr>
  <tr>
    <td>Ethernet Cable</td>
    <td>WIRED CONNECTION: Network connection for the Pi</td>
    <td style="text-align:right">$4.00</td>
    <td><a target="_blank" href="https://ebay.to/2Naib3V">Purchase</a></td>
  </tr>
</table>

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

## From Host Device ELL to Raspberry Pi
The next step is to now get a compressed model from the ELL Library from your laptop to your Raspberry Pi to be utilized. The directions below are modified based on a tutorial sample from the site [Microsft-Ell-Image-Classification.](https://microsoft.github.io/ELL/tutorials/Getting-started-with-image-classification-on-the-Raspberry-Pi/)

1. Activate your Python enviroment on your host device
```
[Linux/macOS] source activate py36
[Windows] activate py36
```
2. Create a new directory to be copied over to the Rasberry Pi. The directory should contain should compressed CTNK or Darknets model in ***model.ell*** form. It should also contain the text file for the model classification
3. As before, run the wrap tool on your laptop or desktop computer, but this time specify the target platform as pi3. This tells the ELL compiler to generate machine code for the Raspberry Pi’s ARM Cortex A53 processor. This step needs to be performed in the directory to be copied to the Raspberry Pi.
```bash
python <ELL-root>/tools/wrap/wrap.py model.ell --language python --target pi3
```
4. To speed up the transfer of files to the Raspberry Pi, delete the model.ell file first before copying the folder. Now, there’s a pi3 directory that contains a CMake project that builds the Python wrapper and some helpful Python utilities. This directory should be inside a directory that also contains the model classification text.

**Build Python Module:**
1. For this step, you’ll be working with your the Raspberry Pi device. If your Pi device is accessible over the network, copy the directory using the Unix scp tool or the Windows WinSCP tool.
2. Log in to your Raspberry Pi, find the directory you just copied from your computer, and build the python module that wraps the ELL model.
```bash
cd pi3
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ../..
```
3. You just created a Python module named model, which includes functions that report the model’s input and output dimensions and makes it possible to pass images to the model for classification.

## Azure Storage
**Azure Blob Storage:** 

The Raspberry Pi has a small storage capability. Therefore, it is important to save picture, videos, models, and project description on the Cloud. For this project, Azure Storage is being used. Steps on how to set up Azure Storage is linked here. [Azure Storage](https://docs.microsoft.com/en-us/azure/storage/common/storage-create-storage-account)

***Note:*** Be sure to save and make note of your **STORAGE ACCOUNT NAME** and **STORAGE ACCOUNT KEYS** They will be needed in the Edge.py script later.
![azureblobs](https://user-images.githubusercontent.com/24871485/42781252-76f83098-88fa-11e8-8f5c-5f5eff0a5c04.PNG)

**Pi3 Folder:**

The utilization of Azure Storage is essential for this application. Azure allows for automatic uploads and downloads of content file. As well, it is neccesarry for persitant updates to the current pi3 folder. 

The first time, the application is ran with a correct Azure Credentials, blob containers are created for use with that given key. As well, the current version of the ***pi3*** folder is uploaded to its respective blob container. Constant checks are being made for changes and updates that occur every 3 hours. 

**Important Note on the Pi3 Folder on Azure:**

After the project has been ran once and the given storage containers have been made, the user can now make changes to the given model and the pi3 folder. 
1. Using the Azure Portal or Microsoft Azure Storage Explorer, locate the **edgemodels** blob container.
2. This is where the compiled "pi3 folder" with its given model is stored. It is important that the pi3 folder is zipped before being ready to be uploaded to the given blob container.
3. There can only be one item in this blob container and it most be titled ***zippedpi3*** for use on the Raspberry Pi.

![edgmodels](https://user-images.githubusercontent.com/24871485/42782127-dcbcfb96-88fc-11e8-8a09-6576447ef46a.PNG)


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
5. If the model and python script recognize an object, a video is captured of the 10 seconds before that moment and 15 seconds after the given moment and then saved it to an **Azure Blob Storage** account.
