# GAPS Channel Demo

## Current version
* **0.0.2**

## Design Goals

Provide a simple and flexible framework for
* Validating GAPS channel API and functionality
* Continuous integration of the PIRATE library
* Facilitating hardware integration
* Measuring and analyzing channel reliability and performance
* Debugging

## Building and Testing
Dependencies
* ```cmake``` version **3.5** or higher
* ```clang```

```
$ git clone https://github.com/GaloisInc/pirate.git
$ cd pirate
$ mkdir build
$ cd build
$ cmake -DCHANNEL_DEMO=ON ..
$ make
```

Output artifacts are located in ```pirate/build/demos/channel_demo```

### Smoke Test
Test the functionality with a GAPS unix pipe channel
```
$ ./reader -l 10,20,1 -C pipe,/tmp/gaps.channel -v
```
in a separate terminal window
```
$ ./writer -l 10,20,1 -C pipe,/tmp/gaps.channel -v
```

## Implementation Overview
The GAPS channel demo contains two binary applications:

### Writer
```writer``` - writes deterministic test data to a GAPS channel

Test data source options
 * Zeros - ```0x00, 0x00, 0x00, ...```
 * Ones - ```0xFF, 0xFF, 0xFF, ...```
 * Incrementing pattern - ```0x00, 0x01, 0x02, ...```
 * User-provided binary blob

#### Test data length options
All auto-generated patterns, ones, zeros, and incrementing pattern can be sent
multiple times with varying length values. Specifically,
in the range of [```start```, ```stop```) with the ```step``` increment.

### Reader
```reader``` - reads and optionally validates deterministic data from a GAPS
channel. If the reader encounters data length or content mismatch between read
and expected data, then both data sets are saved to binary files for further
inspection and debugging.

### Command-Line Options
Both ```writer``` and ```reader``` are always invoked with the same command-line
options.

#### GAPS Channel Selection
GAPS channel type and parameters are passed with the ```-C``` option

```-C channel,options,...  ```

Use the ```--help``` command-line option to view the list of supported GAPS
channels.

#### Test Data Type and Length Selection
Auto-generated patterns are selected with the ```-p``` option:
* **Zeros** - ```-p zeros```
* **Ones** - ```-p onces```
* **Incrementing pattern** - ```-p incr```

Auto-generated test patterns can be sent repetitively with increasing length
sizes with the ```-l``` option

```-l start,stop,step```

```stop``` and ```step``` are optional

For example
 * ```-l 10,100,2``` would send messages with lengths ```10,12,...,98```
 * ```-l 10,100``` would send messages with lengths ```10,11,...,99```
 * ```-l 10``` would send a test single test messages with length ```10```

The default message length is ```100``` bytes

User may provide a binary file path that can be used for test message content

```-i <path>```

User-provided binary messages are sent as a whole

#### Continuous Mode, Inter-Message Delay, and Message Flood Options
Use ```-d ns``` option to set inter-message delay in nanoseconds. The default
is ```1,000,000,000 ns```.

By default the writer stops sending test messages after the end of the test
length sequence or after a single user-provided binary message. The ```-c```
option puts the writer into a continuous mode, which loops over the test
sequence.

The ```-f``` or flood option is equivalent to

```-d 0 -c```

#### Saving Test Messages
Writer and reader test messages can be saved to a specified directory with the

```-s <dir>```

option. If the save option is not specified and the ```reader``` encounters an
expected input mismatch then expected input message and received input message
are saved in the current working directory.

#### Output Verbosity
Each ```-v``` increases output verbosity:
 * Default verbosity initial header is displayed only
 * ```-v``` writer and reader print event messages for each sent or received
  message
 * ```-vv``` all of the above with addition of printed message content

## Channel Reliability and Performance Testing
### Typical Usage
```
./writer -C <channel configuration> -P,<packet length>,<packet count> -d <inter-packet delay in ns> -v
./reader -C <channel configuration> -P,<packet length>,<packet count> -d <inter-packet delay in ns> -v
```
The writer sends ```packet count``` number of packets with the specified
delay in nanoseconds. Each message payload length is ```packet length``` bytes
long. The first 4 bytes in a packet contain an incrementing sequence number.

The reader allocates a zero-filled buffer of ```packet count``` bytes long and
listens for incoming packets. Upon arrival of a packet the byte at
```offset = packet index``` in the buffer is incremented.  With an arrival of
all packets or a timeout of **5 seconds** the reader saves the buffer to a
binary file and exits.

 * If all packets were received, then all bytes in the file will be 0x01
 * If a packet was not received, the byte at the ```offset = packet index```
   will be 0x00
 * If, for some reason, a packet was received twice the count will be 0x02

Binary file name format:

```perf_<packet length>_<packet count>_<delay in ns>_<date>_<time>.bin```

Do not rename the output file since the python script that analyzes the counts
relies on this format.

### Reliability Analysis
The ```perf.py``` Python script performs basic analysis and plots channel
reliability for multiple packet count binary files.

#### Dependencies
* ```numpy```
* ```matplotlib```

#### Usage
```
./perf.py --help
usage: perf.py [-h] [-n INTERVALS] inputs [inputs ...]

Run performance analysis on the captured data counters

positional arguments:
  inputs                Input counts binary files

optional arguments:
  -h, --help            show this help message and exit
  -n INTERVALS, --intervals INTERVALS
                        Number of averaged intervals
```

#### Examples
##### Multi-Plot With Averaged Intervals
 ![Alt text](res/channel_demo_multi.png?raw=true "Multi-Plot With Averaged Intervals")

##### Single Plot Without Averaging
 ![Alt text](res/channel_demo_single.png?raw=true "Single Plot Without Averaging")

# Usage
## Writer Command-Line Options
```
Usage: writer [OPTION...] args ...
Test utility for writing deterministic GAPS packets

  -c, --cont                 Operate in a continuous loop
  -C, --channel=CH           <channel options>
  -d, --delay=NS             Inter-packet delay
  -i, --input=PATH           Binary input file path
  -l, --length=LEN           Test lengths <START,STOP,STEP>
  -p, --pattern=PAT          Test pattern (zeros,onces,incr)
  -P, --perf=PERF            Performance test: <MSG_LEN,COUNT>
  -s, --save=DIR             Save test packets in a directory
  -v, --verbose              Increase verbosity level

 Supported channels:
   DEVICE        device,path[,min_tx_size=N,mtu=N]
   PIPE          pipe,path[,min_tx_size=N,mtu=N]
   UNIX SOCKET   unix_socket,path[,buffer_size=N,min_tx_size=N,mtu=N]
   TCP SOCKET    tcp_socket,reader addr,reader port,writer addr,writer port[,buffer_size=N,min_tx_size=N,mtu=N]
   UDP SOCKET    udp_socket,reader addr,reader port,writer addr,writer port[,buffer_size=N,mtu=N]
   SHMEM         shmem,path[,buffer_size=N,mtu=N]
   UDP_SHMEM     udp_shmem,path[,buffer_size=N,packet_size=N,packet_count=N,mtu=N]
   UIO           uio[,path=N,mtu=N]
   SERIAL        serial,path[,baud=N,mtu=N]
   MERCURY       mercury,mode=[immediate|payload],session=N,message=N,data=N[,descriptor=N,mtu=N]
   GE_ETH        ge_eth,reader addr,reader port,writer addr,writer port,msg_id[,mtu=N]


  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

## Reader Command-Line Options
```
Usage: reader [OPTION...] args ...
Test utility for reading deterministic GAPS packets

  -c, --cont                 Operate in a continuous loop
  -C, --channel=CH           <channel options>
  -d, --delay=NS             Inter-packet delay
  -i, --input=PATH           Binary input file path
  -l, --length=LEN           Test lengths <START,STOP,STEP>
  -p, --pattern=PAT          Test pattern (zeros,onces,incr)
  -P, --perf=PERF            Performance test: <MSG_LEN,COUNT>
  -s, --save=DIR             Save test packets in a directory
  -v, --verbose              Increase verbosity level

 Supported channels:
   DEVICE        device,path[,min_tx_size=N,mtu=N]
   PIPE          pipe,path[,min_tx_size=N,mtu=N]
   UNIX SOCKET   unix_socket,path[,buffer_size=N,min_tx_size=N,mtu=N]
   TCP SOCKET    tcp_socket,reader addr,reader port,writer addr,writer port[,buffer_size=N,min_tx_size=N,mtu=N]
   UDP SOCKET    udp_socket,reader addr,reader port[,buffer_size=N,mtu=N]
   SHMEM         shmem,path[,buffer_size=N,mtu=N]
   UDP_SHMEM     udp_shmem,path[,buffer_size=N,packet_size=N,packet_count=N,mtu=N]
   UIO           uio[,path=N,mtu=N]
   SERIAL        serial,path[,baud=N,mtu=N]
   MERCURY       mercury,mode=[immediate|payload],session=N,message=N,data=N[,descriptor=N,mtu=N]
   GE_ETH        ge_eth,reader addr,reader port,writer addr,writer port,msg_id[,mtu=N]


  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version
```

## Hardware Integration

This section provides useful locations (filenames and function names)
for integrating hardware devices.

### GE Ethernet
#### Channel Configuration and Initialization
Command-line option processing:
```
pirate/libpirate/ge_eth.c/pirate_ge_eth_parse_param()
pirate/libpirate/ge_eth.c/pirate_ge_eth_open()
```
#### Write
GE Ethernet header packing and socket send
```
pirate/libpirate/ge_eth.c:ge_message_pack()
pirate/libpirate/ge_eth.c:pirate_ge_eth_write()
```

#### Read
GE Ethernet header unpacking and socket received
```
pirate/libpirate/ge_eth.c:ge_message_unpack()
pirate/libpirate/ge_eth.c:pirate_ge_eth_read()
```

### Mercury
#### Channel Configuration and Initialization
Command-line option processing:

```
pirate/libpirate/ge_eth.c/pirate_mercury_parse_param()
pirate/libpirate/mercury.c/pirate_mercury_open()
```

#### Write
```
pirate/libpirate/mercury.c:mercury_message_pack()
pirate/libpirate/mercury.c:pirate_mercury_write()
```

#### Read
```
pirate/libpirate/mercury.c:mercury_message_unpack()
pirate/libpirate/mercury.c:pirate_mercury_read()
```
