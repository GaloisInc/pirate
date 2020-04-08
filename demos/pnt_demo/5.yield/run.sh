#!/bin/sh
set -e
./pnt_green_yield --gps-to-target "pipe,/tmp/gapsGT,yield=1" --gps-to-uav "pipe,/tmp/gapsGU,yield=1" --uav-to-target "pipe,/tmp/gapsUT,yield=1" --rf-to-target "pipe,/tmp/gapsRT,yield=1" --green-to-orange-control "pipe,/tmp/gapsGO,control=1" --orange-to-green-control "pipe,/tmp/gapsOG,control=1" --duration 200 &
green_pid=$!
./pnt_orange_yield --gps-to-uav "pipe,/tmp/gapsGU,yield=1" --uav-to-target "pipe,/tmp/gapsUT,yield=1" --rf-to-target "pipe,/tmp/gapsRT,yield=1" --green-to-orange-control "pipe,/tmp/gapsGO,control=1" --orange-to-green-control "pipe,/tmp/gapsOG,control=1" --duration 200 &
orange_pid=$!
wait $green_pid
wait $orange_pid
