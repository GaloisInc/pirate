#!/bin/sh
set -e
./green  --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT"&
./yellow --gps-to-uav "pipe,/tmp/gapsGU" --uav-to-target "pipe,/tmp/gapsUT" --rf-to-target "pipe,/tmp/gapsRT"&
sleep 5s