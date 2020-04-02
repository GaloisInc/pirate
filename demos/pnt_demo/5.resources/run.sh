#!/bin/sh
set -e
./pnt_green --gps-to-target "pipe,/tmp/gapsGT" --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --duration 200 &
green_pid=$!
./pnt_orange --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --fixed &
orange_pid=$!
wait $green_pid
wait $orange_pid
