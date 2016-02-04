#!/bin/sh

if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
	sudo apt-get install -y \
	    make \
	    cmake \
	    g++ \
	    protobuf-compiler \
	    libprotobuf-dev \
	    libgoogle-perftools-dev \
	    libboost-python-dev \
	    libeigen3-dev \
	    python-setuptools \
	    cython \
	    python-numpy \
	    python-scipy \
	    graphviz \
	    unzip \
	    #
elif [ "$(uname)" == "Darwin" ]; then
	brew install make
	brew install cmake
	brew install gcc48 --with-fortran
	CC=gcc4.8 CXX=g++-4.8 brew homebrew/versions/protobuf241
	brew install google-perftools
	brew install boost-python
	brew install eigen
	brew install python
	brew install graphviz
	brew install unzip
fi

# install distributions separately
grep -v distributions requirements.txt | xargs pip install
