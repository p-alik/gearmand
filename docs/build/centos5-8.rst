==========
Centos 5.8
==========


A version of Boost >= 1.39 must be installed.

1. Download epel-release-5-4.noarch.rpm (or suitable version) from http://fedoraproject.org/wiki/EPEL#How_can_I_use_these_extra_packages.3F

2. Install the EPEL repo list using "rpm -Uvh epel-release-5-4.noarch.rpm"

3. yum install boost141-devel

4. ln -s /usr/include/boost141/boost/ /usr/include/boost

5. export LDFLAGS="-L/usr/lib64/boost141"

6. export LD_LIBRARY_PATH=/usr/lib64/boost141:$LD_LIBRARY_PATH

7. sudo yum install e2fsprogs-devel e2fsprogs


Then:

1. yum install gcc44 gcc-c++

2. export CC="gcc44"

3. export CXX="g++44"

Compile and install:

./configure

make

sudo make install

