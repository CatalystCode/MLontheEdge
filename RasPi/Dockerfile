FROM resin/rpi-raspbian:stretch-20180626

RUN apt-get update && apt-get install -y --no-install-recommends\
	# to build and install
    	unzip \
    	build-essential cmake pkg-config \
    	# to work with images
    	libjpeg-dev libtiff5-dev libjasper-dev libpng12-dev libdc1394-22-dev \
    	# to work with videos
    	libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
		libxvidcore-dev libx264-dev \
    	libopenblas-dev \
    	liblapacke-dev \
    	# needed by highgui tool
    	#libgtk2.0-dev libgtk-3-dev \
    	# for opencv math operations
    	libatlas-base-dev gfortran \
    	# Python Dev
    	python3-dev \
    	# Python Pip
    	python3-pip \
    	# to build opencv
    	cmake \
    	# Get information
    	curl \
    	wget \
    	# Open CV Requirement
    	python3-numpy 

# Update and Upgrade
RUN apt-get update && apt-get upgrade -y

# Install final neccesarry opencv packages
RUN apt-get install -y libgtk2.0-dev libgtk-3-dev \
	python3-picamera
		
RUN pip3 install jinja2 \
	ipython

# Begin building opencv in a Docker Container
WORKDIR /home/pi
RUN mkdir opencvbuilder
RUN wget -O opencv.zip https://github.com/Itseez/opencv/archive/3.3.0.zip
RUN unzip opencv.zip

WORKDIR /home/pi/opencv-3.3.0
RUN wget -O opencv_contrib.zip https://github.com/Itseez/opencv_contrib/archive/3.3.0.zip 
RUN unzip opencv_contrib.zip

WORKDIR /home/pi/opencv-3.3.0
RUN mkdir build

WORKDIR /home/pi/opencv-3.3.0/build
RUN cmake -D CMAKE_BUILD_TYPE=RELEASE \
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
  -D PYTHON3_NUMPY_INCLUDE_DIRS=usr/local/lib/python3.5/site-packages/numpy/core/include \
  -D PYTHON3_PACKAGES_PATH=/usr/local/lib/python3.5/site-packages \
  ..

RUN make

RUN make install

# Need to export path to the bashrc
RUN echo 'export PYTHONPATH=$PYTHONPATH:/usr/local/lib/python3.5/site-packages' >> ~/.bashrc

# Run the pip install of the Azure IoTHub Clients
WORKDIR /home/pi
RUN pip3 install azure-iothub-device-client 
RUN pip3 install azure-iothub-service-client 
RUN pip3 install azure-storage

# Delete the downloaded opencv helpers
RUN rm -rf opencvbuilder
RUN rm -rf opencv.zip
RUN rm -rf opencv-3.3.0

# Pi Camera Compatilibity
RUN usermod -a -G video root
CMD modprobe bcm2835-v4l2

# Update and Upgrade
RUN apt-get update && apt-get upgrade -y

# Get the AMLonEdge Project in the Docker Container
RUN mkdir amlonedge
WORKDIR /home/pi/amlonedge
COPY ./test.py /home/pi/amlonedge/test.py
COPY ./run.sh /home/pi/amlonedge/run.sh
COPY ./categories.txt /home/pi/amlonedge/categories.txt
COPY ./Edge.py /home/pi/amlonedge/Edge.py
COPY ./ellmanager.py /home/pi/amlonedge/ellmanager.py
COPY ./Host.py /home/pi/amlonedge/Host.py
COPY ./pisetup.py /home/pi/amlonedge/pisetup.py
COPY ./updatehistory.json /home/pi/amlonedge/updatehistory.json

# Add pi3 - Make pi3 directory
RUN mkdir pi3
WORKDIR /home/pi/amlonedge/pi3
COPY ./pi3/ /home/pi/amlonedge/pi3/

# Go back into the running directory
WORKDIR /home/pi/amlonedge

# Run the given shell script when prompted
RUN chmod +x Edge.py
RUN chmod +x pisetup.py

CMD ["/bin/bash", "-c", "chmod +x run.sh && ./run.sh" ]

