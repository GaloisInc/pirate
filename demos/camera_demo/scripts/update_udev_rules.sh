#!/bin/bash

set -x
set -e

# Freespace device
sudo cp ../udev/65-freespace.rules /etc/udev/rules.d

# Update
sudo udevadm control --reload-rules
sudo udevadm trigger
