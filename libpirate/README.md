# libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on GAPS hardware. At minimum, there are four basic capabilities that must
be supported: configuring GAPS hardware, loading code and data onto the
appropriate CPU, implementing channel send and receive calls, and resource
cleanup / data wipe on termination.

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

### PIPE type

```
"pipe,path[,iov_len=N]"
```

Linux named pipes. Path to named pipe must be specified.

### DEVICE type

```
"device,path[,iov_len=N]"
```

The pathname to the character device must be specified.

### UNIX_SOCKET type

```
"unix_socket,path[,iov_len=N,buffer_size=N]"
```

Unix domain socket communication. Path to Unix socket must be specified.

### TCP_SOCKET type

```
"tcp_socket,reader addr,reader port[,iov_len=N,buffer_size=N]"
```

TCP socket communication. Host and port of the reader process must be specified.

### UDP_SOCKET type

```
"udp_socket,reader addr,reader port[,iov_len=N,buffer_size=N]" 
```

UDP socket communication. Host and port of the reader process must be specified.

### SHMEM type

```
"shmem,path[,buffer_size=N]"
```

Uses a POSIX shared memory region to communicate. Support
for the SHMEM type requires the librt.so POSIX real-time extensions
library. This support is not included by default. Set
the PIRATE_SHMEM_FEATURE flag in [CMakeLists.txt](/libpirate/CMakeLists.txt)
to enable support for shared memory.

The SHMEM type is intended for benchmarking purposes only.
The size of the shared memory buffer can be specified using
`pirate_set_shmem_size(int, int)` prior to opening the channel.

### UIO_DEVICE type

```
"uio[,path=N]"
```

Uses shared memory provided by the kernel from a Userspace IO
device driver. The [uio-device](/devices/uio-device/README.md) kernel module
must be loaded.

## Tests

### Dependencies

[Google Test](https://github.com/google/googletest)

```
$ git clone https://github.com/google/googletest.git
$ cd googletest
$ git checkout v1.10.x
$ cmake -DCMAKE_BUILD_TYPE=Release googletest-release-1.10.x .
$ sudo cmake --build . --target install
```

### Build
Enable **PIRATE_UNIT_TEST** option:
```
$ mkdir build
$ cd build
$ cmake -DPIRATE_UNIT_TEST=ON ..
$ make
```

### Run
```
$ cd build
$ ./libpirate/gaps_channels_test
```

### Run with [valgrind](https://valgrind.org/)

```
$ cd build
$ make valgrind
```
