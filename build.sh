#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-build}
BUILD_TYPE=${BUILD_TYPE:-release}
DIR=$BUILD_DIR/$BUILD_TYPE

cd $SOURCE_DIR/deps/libevent
test -f Makefile || ./configure
make

cd $SOURCE_DIR/deps/cassandra/cpp-driver
if [ -d build ]; then
  cd build && cmake .. && make
else
  mkdir build && cd build && cmake .. && make
fi

cd $SOURCE_DIR/deps/mongo-c-driver/build
test -f Makefile || \
  ../configure --enable-static=yes \
               --enable-ssl=no \
               --enable-examples=no \
               --enable-tests=no \
               --enable-sasl=no
make

cd $SOURCE_DIR
test -d $DIR || mkdir -p $DIR 
cd $DIR \
  && cmake \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      $SOURCE_DIR \
  && make $*
