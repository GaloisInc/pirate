## Project structure

### Building
```
$ cd pirate-demos
$ mkdir build
$ cd build
$ cmake ..
$ make
```

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
the [wiki](https://github.com/GaloisInc/pirate-demos/wiki/libpirate-benchmarks).

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
