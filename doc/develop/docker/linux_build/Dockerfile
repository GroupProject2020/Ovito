# How to rebuild the docker image and update the Gitlab registry:
#
# docker login registry.gitlab.com
# docker build -t registry.gitlab.com/stuko/ovito/linux_build .
# docker push registry.gitlab.com/stuko/ovito/linux_build
#

# Base image
FROM ubuntu:latest

# Upgrade the base packages
RUN apt-get update -qy
RUN apt-get upgrade -qy

# Install packages that are needed for building the code
RUN apt-get install -qy  cmake \
                         git \
                         build-essential \
                         qt5-default \
                         libqt5svg5-dev \
                         libboost-dev \
                         libqt5scintilla2-dev \
                         libavcodec-dev \
                         libavdevice-dev \
                         libavfilter-dev \
                         libavformat-dev \
                         libavresample-dev \
                         libavutil-dev \
                         libswscale-dev \
                         libnetcdf-dev \
                         libssh-dev \
                         libhdf5-dev \
                         libhdf5-serial-dev \
                         libmuparser-dev \
                         libqwt-qt5-dev \
                         python3-dev \
                         python3-sphinx \
                         python3-numpy \
                         python3-sip \
                         python3-pyqt5 \
                         python3-matplotlib \
                         python3-ase \
                         python3-pip \
                         python-dev \
                         python-sphinx \
                         python-numpy \
                         python-sip \
                         python-pyqt5 \
                         python-matplotlib \
                         python-ase \
                         python-pip \
                         xsltproc \
                         docbook-xml \
                         povray \
                         linkchecker

# We need a more recent version of the NumPy package.
RUN pip2 install --upgrade numpy
RUN pip3 install --upgrade numpy
