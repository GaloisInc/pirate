# Installation

These instructions are for Ubuntu. Modify them for your distribution.

```
sudo apt install libfuse-devel
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

# Usage

To build, use `cmake .` then `make`.

To run:

```
./cusegaps_pipe -f --name foobar
```

or

```
./cusegaps_net -f --name foobar --port 12345 --address 127.0.0.1
```

As a demonstration, run in one terminal

```
cat < /dev/foobar
```

and in another terminal

```
echo "hello world" > /dev/foobar
```

# Limitations

Signal handling does not work on blocking operations. When the cuse
device receives a signal in a blocking system call, the signal
is not propagated to the user process. The user process enters
a state such that it cannot be killed. The character device driver
process must be terminated. This has been observed for SIGTERM
and SIGKILL.

More investigation is needed to determine whether this is a FUSE
library bug or we are using the library incorrectly.