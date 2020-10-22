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

## Components

The camera demo consists of four components: a video source, input devices, output devices,
and video frame processors. All of the components are disabled by default. If you start
the camera demo with no arguments then it will do nothing.

### Video Source

The following types of video sources are supported:

* [Raspberry Pi Camera V2](https://www.raspberrypi.org/products/camera-module-v2)
* Laptop or USB webcam
* MPEG-TS H.264 video stream
* no video source

The demo supports exactly one video source, or zero sources if 'no video source' is selected.
The camera sources are capable of generating Motion JPEG, raw YUYV, or H.264 video frames.
If you specify the MPEG-TS H.264 video stream then the video stream url must be specified
with `--decoder=url`.

### Input Devices

The following types of input devices are supported:

* [FSM-9](https://www.ceva-dsp.com/product/fsm-9/) 9-axis inertial measurement unit with an USB interface
* keyboard input with read from left/right arrow keys
* automatic color tracking (both an input device and a frame processor)

The demo supports zero or more input devices.

### Output Devices

The following types of output devices are supported:

* servo motor controlled by a Raspberry Pi GPIO
* software emulation of servo motor position
* no output device

The demo supports exactly one output device, or zero devices if 'no output device' is selected.

### Frame Processor

The following types of frame processors are supported:

* save image frames to the filesystem
* automatic color tracking (both an input device and a frame processor)
* sliding window image filter
* xwindows display
* MPEG-TS H.264 encoding

The demo supports zero or more frame processors.

## GAPS Channels

There are two version of the demo. `camera_demo_monolith` is a single application
that does not use gaps channels. `camera_demo_gaps` can be run as a distribution
application that communicates over gaps channels. The input devices and the output
device communicate over gaps channels. The command line arguments `--gaps_req`
and `--gaps_rsp` specify the gaps channel configuration for request and response
channels, respectively. Each argument can be specify one or more times. The
order of the arguments matters. The first gaps request configuration is paired
with the first gaps response configuration, the second configurations are paired,
etc. The sliding window component requires a response channel.

## Prototype

 ![Alt text](img/embedded_camera_demo.jpg?raw=true "Prototype")

## Usage

```
Usage: camera_demo_monolith [OPTION...]
Embedded application based on camera, position input and position driver

 video options:
  -d, --video_device=device  video device
  -D, --decoder=url          MPEG-TS H.264 decoder url (host:port)
  -f, --flip=v|h             horizontal or vertical image flip
  -H, --height=pixels        image height
  -t, --video_type=type      video type (jpeg|yuyv|h264|stream|none)
  -W, --width=pixels         image width

 frame processor options:
      --codec=type           encoder codec (mpeg1|mpeg2|h264)
  -C, --color_track=RRGGBB   color tracking (RGB hex)
  -E, --encoder=url          MPEG-TS H.264 encoder url (host:port)
  -F, --filesystem           filesystem frame processor
      --out_count=val        image output maximum file count
      --out_dir=path         image output directory
      --sliding              sliding window image filter
      --threshold=val        color tracking threshold
  -X, --xwindows             xwindows frame processor

 input/output options:
      --gaps_req=channel     gaps request channel
      --gaps_rsp=channel     gaps response channel
      --in_freespace         read position input from freespace device
      --in_keyboard          read position input from keyboard
  -o, --output=type          controller output (servo|print|none)
      --output_incr=val      angular position increment
      --output_limit=val     angular position bound

      --loglevel=val         ffmpeg libraries log level
  -v, --verbose              verbose output

  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Example usage:

```
 ./demos/camera_demo/camera_demo_monolith --pos_in kbd --xwindows --sliding --verbose
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
sudo apt install git cmake clang libssl-dev libjpeg-dev libx11-dev libpigpio-dev libusb-1.0-0-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
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
