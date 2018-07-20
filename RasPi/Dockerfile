FROM python:3

ENTRYPOINT []

RUN modprobe bcm2835-v4l2

RUN apt-get update 

RUN apt-get install -y \
	cmake \
	libopenblas-dev \
	curl \

RUN apt-get upgrade -y

RUN apt-get install -y \
	build-essential cmake pkg-config \
	libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev libdc1394-22-dev \
	libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
	libxvidcore-dev libx264-dev \
	libgtk2.0-dev libgtk-3-dev \
	libatlas-base-dev gfortran \
	python3-dev \
	
CMD ["python3", "-m", "install", "jinja2"]

CMD ["python3", "-m", "pip", "install", "numpy"]

WORKDIR /home/pi

CMD ["wget", "-O", "opencv.zip", "https://github.com/Itseez/opencv/archive/3.3.0.zip"]

CMD ["unzip", "opencv.zip"]

WORKDIR /opencv-3.3.0 

CMD ["wget", "-O", "opencv_contrib.zip", "https://github.com/Itseez/opencv_contrib/archive/3.3.0.zip"] 

CMD ["unzip", "opencv_contrib.zip"]

WORKDIR /home/pi/opencv-3.3.0

RUN mkdir -p build

WORKDIR /home/pi/opencv-3.3.0/build

RUN cmake -D CMAKE_BUILD_TYPE=RELEASE \
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

RUN make

RUN make install
