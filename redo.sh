#!/bin/sh
make maintainer-clean
#./get_submodules.sh
./autogen.sh
./configure --prefix=$HOME/usr \
--with-gmp=system \
--with-gmp-prefix=/usr/local \
--with-osl=system \
--with-osl-prefix=$HOME/usr \
--with-scoplib=system \
--with-scoplib-prefix=$HOME/usr \
--with-candl=system \
--with-candl-prefix=$HOME/usr
#./configure --prefix=$HOME/usr
make
