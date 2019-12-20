# libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on TA1 hardware. At minimum, there are four basic primitives that must
be supported: configuring TA1 hardware, loading code and data onto the
appropriate CPU, implementing channel send and receive calls, and resource
cleanup / data wipe on termination.

## Usage

See [primitives.h](/libpirate/primitives.h) for additional documentation.

Reader:

```
  int data;
  if (pirate_open(1, O_RDONLY) < 0) {
    perror("reader open error");
    exit(1);
  }
  if (pirate_read(1, &data, sizeof(data)) != sizeof(data)) {
    perror("read error");
    exit(2);
  }
  pirate_close(1, RD_ONLY);
```

Writer:

```
  int data = 1234;
  if (pirate_open(1, O_WRONLY) < 0) {
    perror("writer open error");
    exit(1);
  }
  if (pirate_write(1, &data, sizeof(data)) != sizeof(data)) {
    perror("write error");
    exit(2);
  }
  pirate_close(1, O_WRONLY);
```

## Channel types

libpirate currently implements GAPS channels using Linux named pipes,
a character device driver, or shared memory. Use
`pirate_set_channel_type(int, channel_t)` to set the channel type.

### PIPE type

Linux named pipes are the default channel type. A pipe file is
created at `/tmp/gaps.channel.%d` if one does not exist.

### DEVICE type

The pathname to the character device must be specified using
`pirate_set_pathname(int, char *)` prior to opening the channel.

### UNIX_SOCKET type

Unix domain socket communication. A unix socket file is
created at `/tmp/gaps.channel.%d.sock` if one does not exist.

### SHMEM type

Uses a POSIX shared memory region to communicate. Support
for the SHMEM type requires the librt.so POSIX real-time extensions
library. This support is not included by default. Set
the PIRATE_SHMEM_FEATURE flag in [CMakeLists.txt](/libpirate/CMakeLists.txt)
to enable support for shared memory.

If the reader or writer process is killed while blocked
on pirate_open() then you must delete the file
`/dev/shm/gaps.channel.%d` prior to launching another reader or writer.

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
of message sizes.

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

Run the unit tests:

```
make test
```

Run the unit tests under valgrind:

```
make valgrind
```
