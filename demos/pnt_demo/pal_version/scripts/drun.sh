#!/bin/bash
# drun.sh path cmd runs cmd inside pirateteam/ubuntu in /root/$path

if [ "$#" == "0" ]; then
	echo "run.sh path cmd"
	exit 1
fi

path=$1
shift
cmd=""
while (($#)); do
	cmd="$cmd $1"
    shift
done

mkdir -p _build
docker run -w /root/$path \
   --rm \
   --mount "type=bind,src=`pwd`,dst=/root/src,ro" \
   --mount "type=bind,src=`pwd`/_build,dst=/root/_build" \
   pirateteam/ubuntu $cmd