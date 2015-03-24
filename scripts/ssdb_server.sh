#!/bin/bash

cd ssdb
if [ $# -eq 0 ]; then
  ./ssdb-server -d ssdb.conf
elif [ "$1" == "stop" ]; then
  ./ssdb-server ssdb.conf -s stop
fi
