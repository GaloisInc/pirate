## PIRATE

This is main software repository for the Pirate distributed-computing
technologies.  Our goal is to develop technologies that help make
it easier to build efficient, safe and secure distributed systems.

The main artifacts available are:

 * The `pirate-llvm` [compiler](https://github.com/GaloisInc/pirate-llvm).
 * The `libpirate` library in this repo, and
 * Demonstration applications that use the above tools.

Trying out the system is very easy -- we provide a [Docker
image](https://hub.docker.com/r/pirateteam/ubuntu) with our tools,
documentation, and demos preinstalled, and provide [online html
documentation](https://pirate-annotations.readthedocs.io).  The docker
image can be installed and run by running

```
docker run -it pirateteam/ubuntu
```

This will download the image from Docker Hub, and start an interactive
bash session in the root account.  The `pirate` directory contains the
demos, and HTML documentaton.  The compiler toolchain and `libpirate`
are pre-installed in `/usr/local`.

If you are not able to run Docker in your environment, we provide
precompiled versions of [`libpirate`](https://github.com/GaloisInc/pirate/actions)
and [`pirate-llvm`](https://github.com/GaloisInc/pirate-llvm/actions)
as part of continuous integration.

Finally, the `pirate-llvm` [repository](https://github.com/GaloisInc/pirate-llvm) has
links to instructions for building it, and instructions for building the
other components are available below.

This material is based upon work supported by the Defense Advanced
Research Projects Agency (DARPA) under Contract No. HR0011-19-C-0103.

### Building

To build the libpirate library:

```
$ cd pirate
$ mkdir build
$ cd build
$ cmake ..
$ make
```

To build the demo applications that use libpirate:

```
$ cd pirate
$ mkdir build
$ cd build
$ cmake -DGAPS_DEMOS=ON ..
$ make
```

#### Additional cmake Options

Invoke with

```
$ cmake -D<OPTION_NAME>=ON ..
```

 * ```PIRATE_UNIT_TEST``` enable compilation of libpirate unit tests (requires googletest v1.10 or greater)
 * ```GAPS_ENABLE``` enable compilation with GAPS annotations
 * ```GAPS_DEMOS``` enable compilation of GAPS demo applications
 * ```GAPS_BENCH``` enable compilation of GAPS benchmark applications
 * ```PIRATE_SHMEM_FEATURE``` support shared memory channels (requires libpthread and librt)
 * ```BUILD_ALL``` enables PIRATE_SHMEM_FEATURE, PIRATE_UNIT_TEST, GAPS_DEMOS, and GAPS_BENCH
 * ```SINGLE_BINARY``` encrypt and combine application binaries into a single executable

### libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on TA1 hardware. At minimum, there are four basic primitives that must
be supported: configuring TA1 hardware, loading code and data onto the
appropriate CPU, implementing channel send and receive calls, and resource
cleanup / data wipe on termination.

libpirate currently implements GAPS channels using Linux named pipes,
a character device driver, a Unix domain socket, shared memory,
network communication, or userspace IO. Benchmarks are available on
the [wiki](https://github.com/GaloisInc/pirate/wiki/libpirate-benchmarks).

### demos

#### simple_demo

Adapted a simple webserver as a demonstration of an application using GAPS
channels. Manually separated the program into two executables. The low side
application is a webserver that sends http requests to the high side
application. The high side application filters the html response before
sending it to the low side application. Uses the libpirate API.

Adapted from http://www.cs.cmu.edu/afs/cs/academic/class/15213-s00/www/class28/tiny.c

The build script for simple_demo includes an experiment that builds
a single executable that contains both the low-side and high-side
executables.

#### time_demo

Trusted timestamping is the process of tracking the time that data was created or modified. A trusted timestamp generally identifies the data that timestamped (typically by a secure hash of the data), the time that the data was timestamped, and a digital signature or other evidence that the timestamp should be trusted. This is typically a digitial signature signed by a trusted third party, but could include additional evidence such as information needed to locate the timestamp in a blockchain ledger. The additional information could be used to provide alternate methods of establishing trust if the private key associated with the digital signature is lost.

### device drivers

#### cusegaps

Implements a character device driver with fifo (named pipe) semantics.
Follows the semantics described in https://docs.google.com/document/d/12hvdts3zsxoWW4KoH26hjaovyZE2s0_LSm_vcw_KL3E/edit which is copied from
http://man7.org/linux/man-pages/man7/pipe.7.html.

Implemented using the CUSE (character device in userspace) functionality
that is available in the FUSE library (https://github.com/libfuse/libfuse).
Communication is implemented using either TCP/IP sockets or named pipes.

#### process-vm-device

This kernel module creates a character device. The character device allows
one reader process and one writer process to communicate using the
[process_vm](https://linux.die.net/man/2/process_vm_writev) interface.
The data moves directly between the address spaces of the two processes,
without passing through kernel space.

#### uio-device

This kernel module creates a userspace IO device. The reader and writer
processes communicate directly through an mmap shared memory region.
The reader and writer do not issue `read()` and `write()` system calls.

### single-binary

The script [platform.py](/single-binary/platform.py) will combine several
executables into a single executable. Each executable is associated with
a unique identifier. The executables are encrypted using the associated
unique identifier. When the resulting program is run, it will execute
one of the input executables based on the matching unique identifier.
