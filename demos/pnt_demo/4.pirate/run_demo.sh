#!/bin/sh
set -e

GPS_TO_UAV=pipe,/tmp/gapsGU
UAV_TO_TGT=mercury,1,1,2
RF_TO_TGT=ge_eth,127.0.0.1,4321,1

MERCURY_DEVICE=/dev/gaps_ilip_0_root
if [ ! -c "${MERCURY_DEVICE}" ]
then
    echo "Mercury device ${MERCURY_DEVICE} not present"
    exit 1
fi

PNT_ORANGE=pnt_orange
PNT_GREEN=pnt_green

./${PNT_ORANGE} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --fixed &
orange_pid=$!
./${PNT_GREEN} --gps-to-uav "${GPS_TO_UAV}" --uav-to-target "${UAV_TO_TGT}" --rf-to-target "${RF_TO_TGT}" --duration 200 &
green_pid=$!

wait $orange_pid

GREEN_STS=$(pgrep ${PNT_GREEN})
if [ ! -z "$GREEN_STS" ]
then
    killall -9 -q ${PNT_GREEN} > /dev/null
fi
wait $green_pid

