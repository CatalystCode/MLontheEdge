# MLontheEdge
The overall purpose of this document is showcase example of Azure Machine Learning on IoT Edge Devices using Microsoft Embedded Learning Library(ELL)

## Table of Contents
1. Windows Device Set Up -- Make Links
* [ELL for Windows Devices](https://github.com/Microsoft/ELL/blob/master/INSTALL-Windows.md) 
2. Set Up for Raspberry Pi Devices -- Make Links
* Python 3.5.3
* Change Hostname
* Camera Set up
* Programming Tools
3. Download PreTrained Model -- Make Links
4. Running Application --Make Links

## Window Device Set Up
The first step is to install Microsoft ELL on your host device. In order to do so, simply follow the directions in the link provided: [MicrosoftELL](https://github.com/Microsoft/ELL/blob/master/INSTALL-Windows.md)

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
Once the picamera is plugged in, a simple test to see if you picamera is working correctly is the command ***raspistill*** :
```bash
raspistill -o image.jpg
```
If a preview window is opened and a new file **image.jpg** is saved, then the picamera is successfully installed.

4. **Network Connection:**
For this project, the majority of the network connectivity came through the attachment of an ethernet cable. However, attached are steps to connecting the Raspberry Pi to a wireless connection. [Wifi Connections](https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md)

5. **Programming Tools:**
**CMake**
CMake will be used on the Raspberry Pi to create python modules that can be called from given code. 


