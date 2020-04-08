#!/bin/sh
set -e

# position
GPS_TO_UAV=mercury,2,2,1,1,3,0,0,0
# position
UAV_TO_TGT=mercury,1,1,2,1,3,0,0,0
# distance
RF_TO_TGT=mercury,1,1,2,1,6,5,0,0

PNT_ORANGE=pnt_orange
PNT_GREEN=pnt_green

./${PNT_ORANGE} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --fixed &
orange_pid=$!
./${PNT_GREEN} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --duration 200 &
green_pid=$!

wait ${green_pid}
kill -9 ${orange_pid}
