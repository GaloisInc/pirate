# Embedded Camera Demo

## Overview
The embedded camera demo is designed to:
* Provide scalable and flexible C++ example for demonstrating GAPS PIRATE capabilities
* Execute on platform-independent embedded environment
* Run on a PC with a webcam if embedded hardware is not available
* Provide independent data producers and consumers that can be placed into different security domains
* Use inexpensive COTS hardware

Figure below illustrates the block diagram
 ![Alt text](img/gaps_pi_demo.png?raw=true "Block Diagram")

### Position Input
Position input generates angular position in degrees:

Input sources
 * [FSM-9](https://www.ceva-dsp.com/product/fsm-9/) 9-axis inertial measurement unit with an USB interface
 * Keyboard input with read from left/right arrow keys
 * New user-derived sources can be created by inheriting the *OrientationInput* class

### Position Output
 * A servo motor controlled by a Raspberry Pi GPIO
 * Pure software implementation when hardware is not available
 * New user-derived position outputs can be created by inheriting the *OrientationOutput* class

### Video Source
 * [Raspberry Pi Camera V2](https://www.raspberrypi.org/products/camera-module-v2)
 * USB webcam

### Frame Processor
 * File system data store
 * New user-derived frame processors can be created by inheriting the *FrameProcessor* class

### Timestamping
 * Currently work in progress
 * Will generate timestamp sign requests processed by the [GAPS timestamp demo](https://github.com/GaloisInc/pirate/tree/master/demos/time_demo)

## Prototype
 ![Alt text](img/embedded_camera_demo.jpg?raw=true "Prototype")

## Usage

```
Usage: camera_demo [OPTION...]
Embedded application based on camera, position input and position driver

 video options:
  -d, --video_device=device  video device
  -f, --flip=v|h             horizontal or vertical image flip
  -H, --height=pixels        image height
  -r, --framerate=num/den    frame rate fraction
  -t, --video_type=jpeg|yuyv video type
  -W, --width=pixels         image width

 frame processor options:
  -c, --color_track=RRGGBB   color tracking (RGB hex) frame processor
  -F, --filesystem           filesystem frame processor
  -m, --monochrome           monochrome image filter
  -M, --out_count=val        image output maximum file count
  -O, --out_dir=path         image output directory
  -s, --sliding              sliding window image filter
  -T, --threshold=val        color tracking threshold
  -X, --xwindows             xwindows frame processor

 input/output options:
  -i, --pos_in=acc|kbd       position input
  -l, --pos_lim=val          angular position bound
  -o, --pos_out=servo|print  angular position output

  -v, --verbose              verbose output

  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Example usage:

```
 ./demos/camera_demo/camera_demo --pos_in kbd --xwindows --sliding --verbose
```

Uses the left and right arrow keys on the keyboard for input
and displays to an XWindow screen for output. The sliding window
filter is applied. The sliding window is controlled by the keyboard.

Additional constraints:

The `--monochrome` filter only works on `yuyv` image types.
It is an example of a filter that is applied on the raw camera data.

The `--threshold` option applies to the `--color_track` frame processor.

The `--sliding` filter only works on the `--xwindows` frame processor.

The `--out_dir` and `--out_count` options apply to the `--filesystem` frame processor.

## Playback MPEG-TS streamer

The following playback settings introduce minimal latency:

```
mpv udp://127.0.0.1:15004  --no-cache --untimed --no-demuxer-thread --video-sync=audio --vd-lavc-threads=1
```

## Raspberry Pi Setup

A Raspberry Pi of any revision should work. This setup has been tested on
Raspberry Pi 3 Model B+

### Configure the Environment

#### Operating System

Download the [Raspberry Pi OS](https://www.raspberrypi.org/downloads/raspberry-pi-os).
Any version of the Raspberry Pi OS is suitable. The initial release of this demo
was based on the May 2020 Lite version.

Unzip the downloaded image:

```
unzip 2020-05-27-raspios-buster-lite-armhf.zip
```

Flash the uncompressed images onto an microSD card. **Caution** double check
the device name of your microSD card since this step may override an your operating
system or another, unintended device.

```
# Locate the correct device
$ lsblk

# Write the OS image
$ sudo dd if=2020-05-27-raspios-buster-lite-armhf.img of=/dev/sdX bs=1M && sync
```

#### Enable SSH

After writing the image mount the boot partition and create an empty **ssh**
file

```
$ cd <mount path>/boot
$ sudo touch ssh
```

Insert the microSD card and power up the device.

#### Find the IP address

```
$ sudo nmap -sS -p 22 <network IP>/24 | grep -B5 Raspberry
```

Example

```
$ sudo nmap -sS -p 22 192.168.42.0/24 | grep -B5 Raspberry

Nmap scan report for raspberrypi (192.168.42.42)
Host is up (0.00039s latency).

PORT   STATE SERVICE
22/tcp open  ssh
MAC Address: B8:27:EB:E8:2C:F1 (Raspberry Pi Foundation)
```

#### SSH Into the Device and Change the Default Password

* Default username **pi**
* Default password **raspberry**

```
$ ssh pi@<ip_address>
$ passwd
```

#### Update and Upgrade

```
$ sudo apt update
$ sudo apt upgrade
```

#### Enable Peripherals

'''
$ sudo raspi-config
'''

* Navigate to **Interfacing Options**
* Enable **Camera**
* Enable **Serial** (Optional)
* Finish
* Reboot

#### Install Dependencies

```
sudo apt install git cmake clang libssl-dev libjpeg-dev libx11-dev libpigpio-dev libusb-1.0-0-dev
```

#### Clone and Build Dependencies

### Freespace Library
```
git clone https://github.com/hcrest/libfreespace.git
$ cd libfreespace
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

## Clone and Build the Camera Demo

### Clone

```
git clone https://github.com/GaloisInc/pirate.git
```

### Update udev Rules

```
$ cd pirate/demos/camera_demo/scripts
$ ./update_udev_rules.sh
```

### Build

```
$ cd pirate
$ mkdir build
$ cd build
$ cmake -DGAPS_DISABLE=ON -DCAMERA_DEMO=ON -DPIRATE_LAUNCHER=OFF ..
```
