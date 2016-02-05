#!/bin/sh

if [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
	sudo apt-get install -y \
	    make \
	    cmake \
	    gcc-4.8 \
	    protobuf-compiler \
#	    libprotobuf-dev \
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
	brew install homebrew/version/gcc48 --with-fortran
	CC=gcc4.8 CXX=g++-4.8 brew install homebrew/versions/protobuf241
	brew install google-perftools
	brew install boost-python
	brew install eigen
	brew install python
	brew install graphviz
	brew install unzip
fi

echo "Now you have to build protobuf (in external_lib) on your own b/c it is an older version"
# install distributions separately
grep -v distributions requirements.txt | xargs pip install
