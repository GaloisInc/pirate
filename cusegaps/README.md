# Installation

These instructions are for Ubuntu. Modify them for your distribution.

```
# create group 'cuse'
sudo addgroup --system cuse
# add yourself to group 'cuse'
sudo usermod -a -G cuse $USER
# create udev rules for /dev/cuse
echo 'KERNEL=="cuse", MODE="0660", GROUP="cuse", OPTIONS+="static_node=cuse"' | sudo tee /etc/udev/rules.d/26-cuse.rules > /dev/null
echo 'ACTION=="add", SUBSYSTEM=="cuse", MODE="0660", GROUP="cuse"' | sudo tee -a /etc/udev/rules.d/26-cuse.rules > /dev/null
# reboot your machine
```

/etc/udev/rules.d/26-cuse.rules should be:
```
KERNEL=="cuse", MODE="0660", GROUP="cuse", OPTIONS+="static_node=cuse"
ACTION=="add", SUBSYSTEM=="cuse", MODE="0660", GROUP="cuse"
```