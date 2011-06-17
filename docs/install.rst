==========
Installing
==========

--------------------------------
Compile and install from tarball
--------------------------------

Download the latest tarball from the download page on Launchpad (https://launchpad.net/gearmand). 

Once downloaded, run::
   tar xzf gearmand-X.Y.tar.gz
   ./configure
   make
   make install


------------------------------------------
Compile and install from source repository
------------------------------------------

The Bazaar version control system is required to check out the latest stable development source. To download and install, run::
   bzr branch lp:gearmand
   cd gearmand
   ./config/autorun.sh
   ./configure
   make
   make install
