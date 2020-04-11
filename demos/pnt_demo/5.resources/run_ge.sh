#!/bin/sh
set -e

GPS_TO_UAV=ge_eth,127.0.0.1,4321,1
UAV_TO_TGT=ge_eth,127.0.0.1,4322,1
RF_TO_TGT=ge_eth,127.0.0.1,4323,2

PNT_ORANGE=pnt_orange
PNT_GREEN=pnt_green

./${PNT_ORANGE} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --fixed &
orange_pid=$!
./${PNT_GREEN} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --duration 200 &
green_pid=$!

wait ${green_pid}
kill -9 ${orange_pid}
