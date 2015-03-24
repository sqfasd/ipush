#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-build}
BUILD_TYPE=${BUILD_TYPE:-release}
DIR=$BUILD_DIR/$BUILD_TYPE
INSTALL_DIR=${INSTALL_DIR:-${DIR}/install}

cd $SOURCE_DIR/deps/libevent && ./configure && make
cd $SOURCE_DIR/deps/ssdb && ./build.sh
cd $SOURCE_DIR

test -d $DIR || mkdir -p $DIR 
cd $DIR \
  && cmake \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
      $SOURCE_DIR \
  && make $*
