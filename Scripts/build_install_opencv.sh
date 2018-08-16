#!/bin/bash

OPENCV_VERSION=3.3.0

WS_DIR=`pwd`
mkdir opencv
cd opencv

# download OpenCV and opencv_contrib
wget -O opencv.zip https://github.com/Itseez/opencv/archive/$OPENCV_VERSION.zip
unzip opencv.zip
rm -rf opencv.zip

wget -O opencv_contrib.zip https://github.com/Itseez/opencv_contrib/archive/$OPENCV_VERSION.zip
unzip opencv_contrib.zip
rm -rf opencv_contrib.zip

OPENCV_SRC_DIR=`pwd`/opencv-$OPENCV_VERSION
OPENCV_CONTRIB_MODULES_SRC_DIR=`pwd`/opencv_contrib-$OPENCV_VERSION/modules

# build and install
cd $OPENCV_SRC_DIR
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE \
  -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D INSTALL_PYTHON_EXAMPLES=ON \
  -D OPENCV_EXTRA_MODULES_PATH=$OPENCV_CONTRIB_MODULES_SRC_DIR \
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

make			

make install
ldconfig

# verify the installation is successful
python -c "import cv2; print('Installed OpenCV version is: {} :)'.format(cv2.__version__))"
if [ $? -eq 0 ]; then
    echo "OpenCV installed successfully! ........................."
else
    echo "OpenCV installation failed :( ........................."
    SITE_PACKAGES_DIR=/usr/local/lib/python2.7/site-packages
    echo "$SITE_PACKAGES_DIR contents: "
    echo `ls -ltrh $SITE_PACKAGES_DIR`
    echo "Note: temporary installation dir $WS_DIR/opencv is not removed!"
    exit 1
fi

# cleanup
cd $WS_DIR
rm -rf opencv