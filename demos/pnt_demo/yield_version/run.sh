#!/bin/sh
set -e
./pnt_yield_green --gps-to-target "pipe,/tmp/gapsGT,src=green,dst=green,listener=1" --gps-to-uav "pipe,/tmp/gapsGU,src=green,dst=orange,listener=1" --uav-to-target "pipe,/tmp/gapsUT,src=orange,dst=green,listener=1" --rf-to-target "pipe,/tmp/gapsRT,src=orange,dst=green,listener=1" --green-to-orange-control "pipe,/tmp/gapsGO,src=green,dst=orange,control=1" --orange-to-green-control "pipe,/tmp/gapsOG,src=orange,dst=green,control=1" --duration 200 &
green_pid=$!
./pnt_yield_orange --gps-to-uav "pipe,/tmp/gapsGU,src=green,dst=orange,listener=1" --uav-to-target "pipe,/tmp/gapsUT,src=orange,dst=green,listener=1" --rf-to-target "pipe,/tmp/gapsRT,src=orange,dst=green,listener=1" --green-to-orange-control "pipe,/tmp/gapsGO,src=green,dst=orange,control=1" --orange-to-green-control "pipe,/tmp/gapsOG,src=orange,dst=green,control=1" --duration 200 &
orange_pid=$!
wait $green_pid
wait $orange_pid
