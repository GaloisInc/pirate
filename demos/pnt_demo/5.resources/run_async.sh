#!/bin/sh
#
# This runs the two endpoints for 5 seconds and then kills them.

set -e
./pnt_green --gps-to-target "pipe,/tmp/gapsGT" --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" &
green_pid=$!
./pnt_orange --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT"&
orange_pid=$!
sleep 5
kill -9 $green_pid
kill -9 $orange_pid