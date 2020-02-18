# GAPS Channel Demo

## Current version
* **0.0.1**

## Design Goals

Provide a simple and flexible framework for
* Validating GAPS channel API and functionality
* Continuous integration of the PIRATE library
* Facilitating hardware integration
* Debugging

## Building and Testing
Dependencies
* ```cmake``` version **3.13** or higher
* ```clang```

```
$ git clone https://github.com/GaloisInc/pirate-demos.git
$ cd pirate-demos
$ mkdir build
$ cd build
$ cmake --target channel_demo ..
$ make writer reader
```

Output artifacts are located in ```pirate-demos/build/demos/channel_demo```

Test the functionality with a GAPS unix pipe channel
```
$ ./reader -l 10,20,1 -C pipe -v
```
in a separate terminal window
```
$ ./writer -l 10,20,1 -C pipe -v
```

## Implementation Overview
The GAPS channel demo contains two components:

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
```reader``` - reads and validates deterministic data from a GAPS channel.
If the reader encounters data length or content mismatch between read and
expected data, then both data sets are saved to binary files for further
inspection and debugging.

### Command-Line Options
Normally ```writer``` and ```reader``` are invoked with the same command-line
options with a few exceptions:
 * Inter-packet write delay
 * Output verbosity

#### GAPS Channel Selection
GAPS channel type and parameters are passed with the ```-C``` option

```-C channel,options,...  ```

##### Supported GAPS Channels
* **PIPE** - ```-C pipe```      
* **UDP Socket** - ``` -C udp,<ip_addr>,<ip_port>```
* **GE Ethernet** - ```- C ge_eth,<ip_addr>,<ip_port>```
* **Mercury** - ``` -C mercury,<path>```

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
Use ```-d us``` option to set inter-message delay in microseconds. The default
is ```1,000,000 us```

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

#### Writer Command-Line Options Options
```
Usage: writer [OPTION...] args ...
Test utility for writing deterministic GAPS packets

  -c, --cont                 Operate in a continuous loop
  -C, --channel=CH           channel,options,...  
  -d, --delay=US             Inter-packet delay
  -i, --input=PATH           Binary input file path
  -l, --length=LEN           START,STOP,STEP
  -p, --pattern=PAT          Test pattern (zeros,onces,incr)
  -s, --save=DIR             Save test packets in a directory
  -v, --verbose              Increase verbosity level

 Supported channels:
     PIPE:          pipe
     UDP SOCKET:    udp,<ip_addr>,<ip_port>
     MERCURY        mercury,<path>
     GE ETHERNET:   ge_eth,<ip_addr>,<ip_port>

  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

```

#### Reader Command-Line Options Options
```
Usage: reader [OPTION...] args ...
Test utility for reading deterministic GAPS packets

  -c, --cont                 Operate in a continuous loop
  -C, --channel=CH           channel,options,...  
  -i, --input=PATH           Binary input file path
  -l, --length=LEN           START,STOP,STEP
  -p, --pattern=PAT          Test pattern (zeros,onces,incr)
  -s, --save=DIR             Save test packets in a directory
  -v, --verbose              Increase verbosity level

 Supported channels:
     PIPE:          pipe
     UDP SOCKET:    udp,<ip_addr>,<ip_port>
     MERCURY        mercury,<path>
     GE ETHERNET:   ge_eth,<ip_addr>,<ip_port>

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
pirate-demos/demos/channel_demo/src/common.c:parse_channel_opt()
pirate-demos/demos/channel_demo/src/common.c:open_gaps_ge_eth()
pirate-demos/demos/channel_demo/src/common.h:CHANNEL_OPT_DESCRIPTION
pirate-demos/libpirate/ge_eth.c/pirate_ge_eth_open()
```
#### Write
GE Ethernet header packing and socket send
```
pirate-demos/libpirate/ge_eth.c:ge_message_pack()
pirate-demos/libpirate/ge_eth.c:pirate_ge_eth_write()
```

#### Read
GE Ethernet header unpacking and socket received
```
pirate-demos/libpirate/ge_eth.c:ge_message_unpack()
pirate-demos/libpirate/ge_eth.c:pirate_ge_eth_read()
```

### Mercury
#### Channel Configuration and Initialization
Command-line option processing:

```
pirate-demos/demos/channel_demo/src/common.c:parse_channel_opt()
pirate-demos/demos/channel_demo/src/common.c:open_gaps_mercury()
pirate-demos/demos/channel_demo/src/common.h:CHANNEL_OPT_DESCRIPTION
pirate-demos/libpirate/mercury.c/pirate_mercury_open()
```

#### Write
```
pirate-demos/libpirate/mercury.c:mercury_message_pack()
pirate-demos/libpirate/mercury.c:pirate_mercury_write()
```

#### Read
```
pirate-demos/libpirate/mercury.c:mercury_message_unpack()
pirate-demos/libpirate/mercury.c:pirate_mercury_read()
```
