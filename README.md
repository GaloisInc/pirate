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
documentation](https://pirate.azureedge.net).  The docker
image can be installed and run by running

```
docker run -it pirateteam/ubuntu
```

This will download the image from Docker Hub, and start an interactive
bash session in the root account.  The `pirate` directory contains the
demos, and HTML documentaton.  The compiler toolchain and `libpirate`
are pre-installed in `/usr/local`.

If you are not able to run Docker in your environment, we provide tarball
archives of the most recent `master` builds for Linux versions:

 * [Centos 7](https://pirate.azureedge.net/dist/pirate-centos7.tgz)
 * [Ubuntu 20.04](https://pirate.azureedge.net/dist/pirate-ubuntu.tgz)

We also upload [artifacts](https://github.com/GaloisInc/pirate/actions)
of our libraries as part of continuous integration.  

If you want to build the source yourself, the `pirate-llvm` [repository](https://github.com/GaloisInc/pirate-llvm) has links to instructions for building it, and instructions for building the other components are available below.

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

To build the channel demo application that uses libpirate:

```
$ cd pirate
$ mkdir build
$ cd build
$ cmake -DCHANNEL_DEMO=ON ..
$ make
```

To build all the demo applications that use libpirate:

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
$ cmake -D<OPTION_NAME>=<OPTON_VALUE> ..
```

 * ```PIRATE_LAUNCHER``` build the application launcher (default ```ON```)
 * ```PIRATE_GETOPT``` build resource loader library (default ```ON```)
 * ```GAPS_DISABLE``` use the standard compiler installed on the system
   instead of the GAPS LLVM compiler (default ```OFF```)
 * ```PIRATE_UNIT_TEST``` enable compilation of libpirate unit tests
   (requires googletest v1.10 or greater, default ```OFF```)
 * ```CHANNEL_DEMO``` enable compilation of GAPS channel application (default ```OFF```)
 * ```GAPS_DEMOS``` enable compilation of all GAPS demo applications (default ```OFF```)
 * ```GAPS_BENCH``` enable compilation of GAPS benchmark applications (default ```OFF```)
 * ```PIRATE_SHMEM_FEATURE``` support shared memory channels
   (requires libpthread and librt, default ```OFF```)
 * ```BUILD_ALL``` enables PIRATE_SHMEM_FEATURE, PIRATE_UNIT_TEST, GAPS_DEMOS,
   and GAPS_BENCH (default ```OFF```)
 * ```SINGLE_BINARY``` encrypt and combine application binaries into a single
   executable (default ```OFF```)

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

#### channel_demo

Provide a simple application for validating GAPS channel API and
functionality, continuous integration of the PIRATE library,
facilitating hardware integration, and debugging. The channel
demo has a reader and a writer that can be configured to use
different channel types and transmit different patterns of data. ([more information](/demos/channel_demo))

#### simple_demo

Adapted a simple webserver as a demonstration of an application using GAPS
channels. Manually separated the program into two executables. The low side
application is a webserver that sends http requests to the high side
application. The high side application filters the html response before
sending it to the low side application. Uses the libpirate API. ([more information](/demos/simple_demo))

Adapted from http://www.cs.cmu.edu/afs/cs/academic/class/15213-s00/www/class28/tiny.c

The build script for simple_demo includes an experiment that builds
a single executable that contains both the low-side and high-side
executables.

#### time_demo

Trusted timestamping is the process of tracking the time that data was created or modified. A trusted timestamp generally identifies the data that timestamped (typically by a secure hash of the data), the time that the data was timestamped, and a digital signature or other evidence that the timestamp should be trusted. This is typically a digitial signature signed by a trusted third party, but could include additional evidence such as information needed to locate the timestamp in a blockchain ledger. The additional information could be used to provide alternate methods of establishing trust if the private key associated with the digital signature is lost. ([more information](/demos/time_demo))
