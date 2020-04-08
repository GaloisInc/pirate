#!/bin/bash

SERIAL_UDEV_RULES=cfg/50-gaps_serial.rules

sudo cp ${SERIAL_UDEV_RULES} /etc/udev/rules.d
sudo udevadm control --reload-rules
sudo udevadm trigger
