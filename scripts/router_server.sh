#!/bin/bash

PROGRAM=router_server
PID_FILE=./${PROGRAM}.pid
CONF_FILE=./conf/${PROGRAM}.conf
LOG_FILE=./logs/${PROGRAM}.log

if [ $# -eq 0 ]; then
  test -e $PID_FILE && echo "faled: server already started !" && exit 1
  ./bin/$PROGRAM -flagfile=$CONF_FILE
elif [ "$1" == "stop" ]; then
  if [ -e $PID_FILE ]; then
    kill $(cat $PID_FILE)
    rm -f $PID_FILE
    echo "server stoped !"
  else
    echo "failed: server not started !" && exit 1
  fi
fi
