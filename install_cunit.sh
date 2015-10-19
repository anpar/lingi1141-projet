#!/bin/bash

tar xjvf CUnit-2.1-3.tar.bz2
cd CUnit-2.1-3
./configure --prefix=$HOME/local
make
make install
