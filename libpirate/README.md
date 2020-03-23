# libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on GAPS hardware. At minimum, there are four basic primitives that must
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

Linux named pipes. Path to named pipe must be specified.

### DEVICE type

The pathname to the character device must be specified
prior to opening the channel.

### UNIX_SOCKET type

Unix domain socket communication. Path to Unix socket must be specified.

### TCP_SOCKET type

TCP socket communication. Host and port must be specified.

### UDP_SOCKET type

UDP socket communication. Host and port must be specified.

### SHMEM type

Uses a POSIX shared memory region to communicate. Support
for the SHMEM type requires the librt.so POSIX real-time extensions
library. This support is not included by default. Set
the PIRATE_SHMEM_FEATURE flag in [CMakeLists.txt](/libpirate/CMakeLists.txt)
to enable support for shared memory.

The SHMEM type is intended for benchmarking purposes only.
The size of the shared memory buffer can be specified using
`pirate_set_shmem_size(int, int)` prior to opening the channel.

### UIO_DEVICE type

Uses shared memory provided by the kernel from a Userspace IO
device driver. The [uio-device](/devices/uio-device/README.md) kernel module
must be loaded.

## Benchmarks

`primitives_bench_thr` and `primitives_bench_lat` are throughput
and latency benchmarks for the library. `bench.py` is a wrapper
script that can be used to run the benchmarks across a range
of message sizes. Benchmarks are compiled using the command
`make bench`.

Example usage:

```
# throughput benchmarks
./bench.py thr unix-pipe >> throughput.results
./bench.py thr device /dev/foobar >> throughput.results
./bench.py thr shmem shmem >> throughput.results

# latency benchmarks
./bench.py lat unix-pipe >> latency.results
./bench.py lat device /dev/foo /dev/bar >> latency.results
./bench.py lat shmem shmem shmem >> latency.results
```

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
