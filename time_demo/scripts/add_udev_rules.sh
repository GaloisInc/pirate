#!/bin/bash

sudo cp gaps.rules /etc/udev/rules.d
sudo udevadm control --reload-rules
sudo udevadm trigger
