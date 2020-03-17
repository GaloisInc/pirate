#!/bin/sh
set -ex
./pnt_green  --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT"&
# --duration 50&
green_pid=$!
./pnt_orange --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT"&
# --duration 50&
orange_pid=$!
wait $green_pid
wait $orange_pid
