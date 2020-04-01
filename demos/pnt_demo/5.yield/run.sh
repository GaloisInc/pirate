#!/bin/sh
set -e
./pnt_green_yield --gps-to-target "pipe,/tmp/gapsGT" --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --green-to-orange-control "pipe,/tmp/gapsGO" --orange-to-green-control "pipe,/tmp/gapsOG" --duration 200 &
green_pid=$!
./pnt_orange_yield --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT" --green-to-orange-control "pipe,/tmp/gapsGO" --orange-to-green-control "pipe,/tmp/gapsOG" --duration 200 &
orange_pid=$!
wait $green_pid
wait $orange_pid
