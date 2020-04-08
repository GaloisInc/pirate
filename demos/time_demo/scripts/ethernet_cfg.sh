#!/bin/bash

LINK_RULES=cfg/*gaps*.link
sudo cp ${LINK_RULES} /etc/systemd/network

# USE WITH CAUTION!
#LINK_RULES=/etc/udev/rules.d/80-net-setup-link.rules
#if [ ! -L ${LINK_RULES} ]
#then
#    sudo ln -s /lib/udev/rules.d/80-net-setup-link.rules ${LINK_RULES}
#    echo "Please reboot to apply this change"
#fi

GAPS_NETPLAN=cfg/02-gaps_net.yaml
sudo cp ${GAPS_NETPLAN} /etc/netplan
sudo netplan apply

