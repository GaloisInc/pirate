#!/bin/bash

# usage: ./camera_demo.sh [device] [args]

MACHINE=`uname --machine`
PLATFORM_ARGS=""
USER_ARGS="${@:2:$#}"
VIDEO_DEVICE="/dev/video0"
PREFIX=""
APP="./camera_demo"

if [ $# -ge 1 ]; then
    VIDEO_DEVICE=$1
fi

if [ ${MACHINE} = "armv7l" ]; then
    PREFIX="sudo"
    PLATFORM_ARGS="-f v -f h"
fi

${PREFIX} ${APP} -d ${VIDEO_DEVICE} ${PLATFORM_ARGS} ${USER_ARGS}
