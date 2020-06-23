#!/bin/sh
# Runs the PNT example with GAPS pipes.

set -e
./pnt_getopt_green  --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --duration 200 &
green_pid=$!
./pnt_getopt_orange --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --fixed &
orange_pid=$!
wait $green_pid
wait $orange_pid
