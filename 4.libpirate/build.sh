#!/bin/sh
set -e

mkdir -p build
docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           -w /root/build \
           pirateteam/ubuntu \
           cmake ../src -DCMAKE_VERBOSE_MAKEFILE=On

docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           -w /root/build \
           pirateteam/ubuntu \
           make

docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           -w /root/build \
           --env LD_LIBRARY_PATH=/usr/local/lib \
           pirateteam/ubuntu \
           /root/src/run.sh
