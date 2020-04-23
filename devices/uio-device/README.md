# uio-device

Device driver that uses Linux [userspace I/O](https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html).
The device allocates four memory regions of 256 contiguous pages (1,048,576 bytes).
Memory regions are allocated from virtual memory (vmalloc).

See [reader.c](/devices/uio-device/reader.c) and [writer.c](/devices/uio-device/writer.c) for simple
examples of how to use the device.

## Install

```
make clean
make
sudo modprobe uio
sudo insmod uio-gaps.ko
sudo chmod 0666 /dev/uio0
```
