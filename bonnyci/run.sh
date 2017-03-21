#!/bin/bash
set -eux
sudo apt-get install -y build-essential libboost-all-dev gperf libevent-dev uuid-dev python-sphinx libhiredis-dev

./bootstrap.sh -a
./configure
make
make test
