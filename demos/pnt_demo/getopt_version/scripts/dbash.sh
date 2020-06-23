#!/bin/bash
# This script runs pirateteam/ubuntu with local directories mounted.

mkdir -p _build
docker run \
   -it --rm \
   --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
   --mount "type=bind,src=`pwd`/_build,dst=/root/_build" \
   pirateteam/ubuntu