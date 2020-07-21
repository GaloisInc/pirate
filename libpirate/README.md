# libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on GAPS hardware. At minimum, there are four basic capabilities that must
be supported: configuring GAPS hardware, loading code and data onto the
appropriate CPU, implementing channel send and receive calls, and resource
cleanup / data wipe on termination.

libpirate is a datagram communication library. Reads and writes are
transmitted in indivisible packets. Reading a partial packet will
drop the remaining contents of the packet.

Nonblocking I/O is supported for the UDP and GE_ETH channel types.
The stream-based channel types (Unix pipe, Unix socket, TCP socket,
etc) cannot support datagram semantics on write requests with
non-blocking I/O. A partial read or partial write of a datagram is
not allowed. This can be prevented on reads using an internal
temporary buffer. The same technique cannot be applied to writes.
Passing O_NONBLOCK to pirate_open() to channel types that do not
support nonblocking I/O will return an errno status of EINVAL.

A Windows implementation of libpirate is provided for development
purposes. The Windows implementation only supports the UDP and
GE_ETH channel types.

The following tables summarizes the capabilities of libpirate
channels.

| Channel type | Linux blocking I/O  | Linux nonblocking I/O | Windows blocking I/O | Windows nonblocking I/O |
| ------------ | ------------------- | --------------------- | -------------------- | ----------------------- |
| device       | Y | | | |
| pipe         | Y | | | | 
| unix_socket  | Y | | | |
| tcp_socket   | Y | | | |
| udp_socket   | Y | Y | Y | Y |
| shmem        | Y | | | |
| uio          | Y | | | |
| serial       | Y | | | |
| mercury      | Y | | | |
| ge_eth       | Y | Y | Y | Y |

## Usage

See [libpirate.h](/libpirate/libpirate.h) for additional documentation.

Reader:

```
  int gd, data;
  gd = pirate_open_parse("pipe,/tmp/gaps", O_RDONLY);
  if (gd < 0) {
    perror("reader open error");
    exit(1);
  }
  if (pirate_read(gd, &data, sizeof(data)) != sizeof(data)) {
    perror("read error");
    exit(2);
  }
  pirate_close(gd);
```

Writer:

```
  int gd, data = 1234;
  gd = pirate_open_parse("pipe,/tmp/gaps", O_WRONLY);
  if (gd < 0) {
    perror("writer open error");
    exit(1);
  }
  if (pirate_write(gd, &data, sizeof(data)) != sizeof(data)) {
    perror("write error");
    exit(2);
  }
  pirate_close(gd);
```

## Channel types

### Common parameters

* mtu - specifies the maximum transmisssion unit. The mtu includes
the length of the packet header. To get the maximum data payload
transmission length, use `pirate_write_mtu()`. Writes longer than
an mtu will generate a EMSGSIZE error.
* min_tx_size - specifies the internal minimum transmission
length for channels that are based on stream tranport. This
parameter is for performance optimization and have no impact
on the semantics of the library. The default value is generally
the value you want to use.
* max_tx_size - specifies the internal maximum transmission
length for channels that are based on stream tranport. This
parameter is for performance optimization and have no impact
on the semantics of the library. The default value is generally
the value you want to use.

### PIPE type

```
"pipe,path[,min_tx_size=N,mtu=N]"
```

Linux named pipes. Path to named pipe must be specified.

### DEVICE type

```
"device,path[,min_tx_size=N,mtu=N]"
```

The pathname to the character device must be specified.

### UNIX_SOCKET type

```
"unix_socket,path[,buffer_size=N,min_tx_size=N,mtu=N]"
```

Unix domain socket communication. Path to Unix socket must be specified.

### TCP_SOCKET type

```
"tcp_socket,reader addr,reader port[,buffer_size=N,min_tx_size=N,mtu=N]"
```

TCP socket communication. Host and port of the reader process must be specified.

### UDP_SOCKET type

```
"udp_socket,reader addr,reader port[,buffer_size=N,mtu=N]"
```

UDP socket communication. Host and port of the reader process must be specified.

### SHMEM type

```
"shmem,path[,buffer_size=N,max_tx_size=N,mtu=N]"
```

Uses a POSIX shared memory region to communicate. Support
for the SHMEM type requires the librt.so POSIX real-time extensions
library. This support is not included by default. Set
the PIRATE_SHMEM_FEATURE flag in [CMakeLists.txt](/libpirate/CMakeLists.txt)
to enable support for shared memory.

### UIO_DEVICE type

```
"uio[,path=N,max_tx_size=N,mtu=N]"
```

Uses shared memory provided by the kernel from a Userspace IO
device driver. The [uio-device](/devices/uio-device/README.md) kernel module
must be loaded.

## Tests

There are separate instructions for Windows below.

### Unix

#### Dependencies

[Google Test](https://github.com/google/googletest)

```
$ git clone https://github.com/google/googletest.git
$ cd googletest
$ git checkout release-1.10.0
$ mkdir build
$ cd build
$ cmake ..
$ sudo cmake --build . --config Release --target install
```

#### Build

Enable **PIRATE_UNIT_TEST** option:
```
$ mkdir build
$ cd build
$ cmake -DPIRATE_UNIT_TEST=ON ..
$ make
```

#### Run

```
$ cd build
$ ./libpirate/gaps_channels_test
```

#### Run with [valgrind](https://valgrind.org/)

```
$ cd build
$ make valgrind
```

### Windows

#### Dependencies

[Google Test](https://github.com/google/googletest)

```
$ git clone https://github.com/google/googletest.git
$ cd googletest
$ git checkout release-1.10.0
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build . --config Release --target install
```
#### Build

Enable **PIRATE_UNIT_TEST** option:

```
$ mkdir build
$ cd build
$ cmake -DGTEST_LIBRARY="C:\\Program Files (x86)\\googletest-distribution\\lib\\gtest.lib" `
    -DGTEST_MAIN_LIBRARY="C:\\Program Files (x86)\\googletest-distribution\\lib\\gtest_main.lib" `
    -DGTEST_INCLUDE_DIR="C:\\Program Files (x86)\\googletest-distribution\\include" `
    -DGAPS_DISABLE=ON `
    -DPIRATE_UNIT_TEST=ON `
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON `
    -DPIRATE_LAUNCHER=OFF `
    -DPIRATE_GETOPT=OFF `
    ..
$ cmake --build . --config Release
```

#### Run

```
$ cd build
$ ./libpirate/Release/gaps_channels_test.exe
```
