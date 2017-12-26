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
                         libfftw3-dev \
                         libhdf5-dev \
                         libhdf5-serial-dev \
                         libbotan1.10-dev \
                         libmuparser-dev \
                         libqwt-qt5-dev \
                         python3-dev \
                         python3-sphinx \
                         python3-numpy \
                         python3-sip \
                         python3-pyqt5 \
                         python3-matplotlib \
                         python3-ase \
                         python-dev \
                         python-sphinx \
                         python-numpy \
                         python-sip \
                         python-pyqt5 \
                         python-matplotlib \
                         python-ase \
                         xsltproc \
                         docbook-xml \
                         povray \
                         linkchecker
