#!/bin/sh
set -e
/root/dist/bin/pnt_green  --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT"&
/root/dist/bin/pnt_orange --gps-to-uav "/tmp/gapsGU" --uav-to-target "/tmp/gapsUT" --rf-to-target "/tmp/gapsRT"&
sleep 5s
