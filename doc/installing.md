# Installing Loom + Distributions

Loom targets Ubuntu 12.04 and 14.04 (with gcc-4.8) systems and requires the
[distributions](https://github.com/posterior/distributions) library.
This guide describes how to install both loom and distributions.

*WARNING* loom does not work with gcc-4.9, so on ubuntu 14.04 systems,

    export CC=gcc-4.8
    export CXX=g++-4.8

## Installing with virtualenvwrapper (recommended)

    # 1. Make a new virtualenv named 'loom'.
    sudo apt-get install -y virtualenvwrapper
    source ~/.bashrc                           # pulls 'mkvirtualenv' into path
    mkvirtualenv --system-site-packages loom

    # 2. Set environment variables.
    echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$VIRTUAL_ENV/lib' >> $VIRTUAL_ENV/bin/postactivate
    echo 'export DISTRIBUTIONS_USE_PROTOBUF=1' >> $VIRTUAL_ENV/bin/postactivate
    workon loom     # pulls the above definitions into current environment

    # 3. Clone the repos.
    git clone https://github.com/posterior/distributions
    git clone https://github.com/posterior/loom

    # 4. Install required packages.
    sudo easy_install pip
    # on mac use homebrew, however you'll want to compile protobuf by hand
    # to ensure it is compiled agaisnt gcc 4.8 (see above)
    source loom/requirements.sh     # uses apt and pip

    # 5. Build distributions.
    cd distributions
    make && make install
    cd ..

    # 6. Build loom.
    cd loom
    make && make install
    make test               # optional, takes ~30 CPU minutes
    cd ..

Make sure to `workon loom` whenever you start a new bash session for looming.

## Installing globally for all users

If you prefer to avoid using virtualenvwrapper:

1.  Set environment variables.

    echo 'export DISTRIBUTIONS_USE_PROTOBUF=1' >> ~/.bashrc
    source ~/.bashrc

3. Build distributions and loom as above, but installing as root

    sudo make install       # instead of `make install`

## Custom Installation

Loom assumes distributions is installed in a standard location.
You may need to set `CMAKE_PREFIX_PATH` for loom to find distributions.

### virtualenv

Within a virtualenv, both distributions and loom assume a prefix of
`$VIRTUAL_ENV`. `make install` installs headers to
`$VIRTUAL_ENV/include`, libs to `$VIRTUAL_ENV/lib`, and so on.

For distributions and loom to find these installed libraries at
runtime, `LD_LIBRARY_PATH` must include `$VIRTUAL_ENV/lib`. With
virtualenvwrapper, it's convenient to do this in a postactivate hook:

    echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$VIRTUAL_ENV/lib' >> $VIRTUAL_ENV/bin/postactivate
