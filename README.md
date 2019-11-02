## Project structure

### libpirate

Pirate primitives layer. The PIRATE core primitives layer
will provide a series of capabilities for executing PIRATE executables
on TA1 hardware. At minimum, there are four basic primitives that must
be supported: configuring TA1 hardware, loading code and data onto the
appropriate CPU, implementing channel send and receive calls, and resource
cleanup / data wipe on termination.

libpirate currently implements GAPS channels in software using Linux named
pipes.

### simple_demo

Adapted a simple webserver as a demonstration of an application using GAPS
channels. Manually separated the program into two executables. The low side
application is a webserver that sends http requests to the high side
application. The high side application filters the html response before
sending it to the low side application. Uses the libpirate API.

Adapted from http://www.cs.cmu.edu/afs/cs/academic/class/15213-s00/www/class28/tiny.c

TODO refactor the source so that the program structure is amenable to pirate
annotations.

The build script for simple_demo includes an experiment that builds
a single executable that contains both the low-side and high-side
executables.

### cusegaps

Implements a character device driver with fifo (named pipe) semantics.
Follows the semantics described in https://docs.google.com/document/d/12hvdts3zsxoWW4KoH26hjaovyZE2s0_LSm_vcw_KL3E/edit which is copied from
http://man7.org/linux/man-pages/man7/pipe.7.html.

Implemented using the CUSE (character device in userspace) functionality
that is available in the FUSE library (https://github.com/libfuse/libfuse).
Communication is implemented using either TCP/IP sockets or named pipes.
