#!/bin/sh
set -e

mkdir -p build
mkdir -p dist
docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           --mount "type=bind,src=`pwd`/dist,dst=/root/dist" \
           -w /root/build \
           pirateteam/ubuntu \
           cmake ../src -DCMAKE_VERBOSE_MAKEFILE=On \
                 -DCMAKE_EXE_LINKER_FLAGS=-static \
                 -DCMAKE_INSTALL_PREFIX=/root/dist

docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           -w /root/build \
           pirateteam/ubuntu \
           make

docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/build,dst=/root/build" \
           --mount "type=bind,src=`pwd`/dist,dst=/root/dist" \
           -w /root/build \
           pirateteam/ubuntu \
           make install

docker run --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
           --mount "type=bind,src=`pwd`/dist,dst=/root/dist" \
           -w /root/dist/bin \
           --env LD_LIBRARY_PATH=/usr/local/lib \
           pirateteam/ubuntu \
           /root/src/run.sh
