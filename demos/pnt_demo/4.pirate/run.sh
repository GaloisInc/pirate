#!/bin/sh
set -e
./pnt_green  --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT" --duration 200&
green_pid=$!
./pnt_orange --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT" --duration 200&
orange_pid=$!
wait $green_pid
wait $orange_pid
