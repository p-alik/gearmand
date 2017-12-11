sudo apt-get update -qq
COMPILER_PACKAGE=""
if [ "$COMPILER" = "g++-4.9" ]; then COMPILER_PACKAGE=g++-4.9
elif [ "$COMPILER" = "g++-5" ]; then COMPILER_PACKAGE=g++-5; fi
sudo apt-get install -y libboost-all-dev gperf libevent-dev uuid-dev python-sphinx libhiredis-dev $COMPILER_PACKAGE
if [ "$COMPILER" = "g++-4.9" ]; then export CXX="g++-4.9" CC="gcc-4.9"
elif [ "$COMPILER" = "g++-5" ]; then export CXX="g++-5" CC="gcc-5"; fi
