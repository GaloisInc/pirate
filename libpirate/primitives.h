/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#ifndef __PIRATE_PRIMITIVES_H
#define __PIRATE_PRIMITIVES_H

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "shmem_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIRATE_FILENAME "/tmp/gaps.channel.%d"
#define PIRATE_DOMAIN_FILENAME "/tmp/gaps.channel.%d.sock"
#define PIRATE_PORT_NUMBER 26427
#define PIRATE_SHM_NAME "/gaps.channel.%d"
#define PIRATE_LEN_NAME 64

#define PIRATE_NUM_CHANNELS 16

#define PIRATE_IOV_MAX 16

typedef enum {
  // The gaps channel is implemented using a FIFO special file
  // (a named pipe). The name of the pipe is formatted
  // with PIRATE_FILENAME where %d is the gaps descriptor.
  PIPE,
  // The gaps channel is implemented using a filepath.
  // This filepath points to a character device or a
  // named pipe. pirate_set_pathname(int, char *) must
  // be used to specify the pathname.
  DEVICE,
  // The gaps channel is implemented by using Unix domain sockets.
  // The name of the socket is formatted with PIRATE_DOMAIN_FILENAME
  // where %d is the gaps descriptor.
  UNIX_SOCKET,
  // The gaps channel is implemented by using TCP sockets.
  // The writer must use pirate_set_pathname(int, char *)
  // to specify the hostname.
  // The port number is specified using pirate_set_port_number(int, int)
  // or defaults to (26427 + d) where d is the gaps descriptor.
  TCP_SOCKET,
  // The gaps channel is implemented by using UDP sockets.
  // The writer must use pirate_set_pathname(int, char *)
  // to specify the hostname.
  // The port number is specified using pirate_set_port_number(int, int)
  // or defaults to (26427 + d) where d is the gaps descriptor.
  UDP_SOCKET,
  // The gaps channel is implemented using shared memory.
  // This feature is disabled by default. It must be enabled
  // by setting PIRATE_SHMEM_FEATURE in CMakeLists.txt
  SHMEM,
  // The gaps channel is implemented using UDP packets
  // transmitted over shared memory. For measuring the
  // cost of creating UDP packets in userspace.
  // This feature is disabled by default. It must be enabled
  // by setting PIRATE_SHMEM_FEATURE in CMakeLists.txt
  SHMEM_UDP,
  // The gaps channel is implemented using userspace io.
  // The gaps uio device driver must be loaded.
  UIO_DEVICE,
  // The gaps channel is implemented over a /dev/tty* interface
  // Baud rate = 230400, no error detection or correction
  SERIAL,
  // The gaps channel for Mercury System PCI-E device
  // Maximum packet size 256 bytes
  // Maximim data size 236 bytes
  MERCURY,
  // The gaps channel for GRC Ethernet devices
  GE_ETH,
  // The gaps channel is unavailable for operations
  INVALID
} channel_t;

typedef struct {
  int fd;                       // file descriptor
  channel_t channel;            // channel type
  char *pathname;               // optional device path
  int port_number;              // optional port number (TCP_SOCKET or UDP_SOCKET)
  int buffer_size;              // optional memory buffer size
  size_t packet_size;           // optional packet size (SHMEM_UDP)
  size_t packet_count;          // optional packet count (SHMEM_UDP)
  shmem_buffer_t *shmem_buffer; // optional shared memory buffer (SHMEM)
  size_t iov_len;               // optional use readv/writev
} pirate_channel_t;

//
// API
//

// Opens the gaps channel specified by the gaps descriptor.
//
// Channels must be opened in order from smaller to largest
// gaps descriptor. pirate_open() will block until both the
// reader and the writer have opened the channel.

// The return value is the input gaps descriptor, or -1 if an
// error occurred (in which case, errno is set appropriately).
//
// The argument flags must be O_RDONLY or O_WRONLY.
int pirate_open(int gd, int flags);

// pirate_read() attempts to read up to count bytes from
// gaps descriptor gd into the buffer starting at buf.
//
// On success, the number of bytes read is returned.
// On error, -1 is returned, and errno is set appropriately.
ssize_t pirate_read(int gd, void *buf, size_t count);

// pirate_write() writes up to count bytes from the buffer
// starting at buf to the gaps descriptor gd.
//
// On success, the number of bytes written is returned
// (zero indicates nothing was written). On error,
// -1 is returned, and errno is set appropriately.
ssize_t pirate_write(int gd, const void *buf, size_t count);

// Closes the gaps channel specified by the gaps descriptor.
//
// pirate_close() returns zero on success.  On error,
// -1 is returned, and errno is set appropriately.
int pirate_close(int gd, int flags);

//
// CONFIGURATION PARAMETERS
//

// Sets the channel type for the read and write ends
// of the gaps descriptor. Must be configured before
// the channel is opened. Returns zero on success.
// On error -1 is returned, and erro is set appropriately.
int pirate_set_channel_type(int gd, channel_t channel_type);

// Gets the channel type for the read and write ends
// of the channel descriptor. Returns INVALID on error.
channel_t pirate_get_channel_type(int gd);

// Sets the pathname or hostname for the read and write ends
// of the gaps descriptor. Only valid if the channel
// type is DEVICE, TCP_SOCKET, or UDP_SOCKET. Returns zero on success.
// On error -1 is returned, and errno is set appropriately.
// If pathname is NULL, then the memory allocated
// for the channel pathname is free'd.
int pirate_set_pathname(int gd, const char *pathname);

// Gets the pathname or hostname for the read and write ends
// of the gaps descriptor. There must be PIRATE_LEN_NAME
// bytes allocated at pathname. Returns zero on success.
// On error -1 is returned, and errno is set appropriately.
int pirate_get_pathname(int gd, char *pathname);

// Sets the port number for the read and write ends
// of the gaps descriptor. Only valid if the channel
// type is TCP_SOCKET or UDP_SOCKET. Returns zero on success.
// On error -1 is returned, and errno is set appropriately.
// If pathname is NULL, then the memory allocated
// for the channel pathname is free'd.
int pirate_set_port_number(int gd, int port);

// Gets the port number for the read and write ends
// of the gaps descriptor. On error -1 is returned,
// and errno is set appropriately.
int pirate_get_port_number(int gd);

// Sets the memory buffer size for the gaps channel.
// Only valid if the channel type is SHMEM or UNIX_SOCKET.
// If zero then SHMEM will allocate DEFAULT_SHMEM_BUFFER bytes.
// On error -1 is returned, and errno is set appropriately.
int pirate_set_buffer_size(int gd, int buffer_size);

// Gets the shared memory buffer size for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
int pirate_get_buffer_size(int gd);

// Sets the packet size for the gaps channel.
// Only valid if the channel type is SHMEM_UDP.
// If zero then SHMEM will allocate DEFAULT_PACKET_SIZE packet size.
// On error -1 is returned, and errno is set appropriately.
int pirate_set_packet_size(int gd, size_t packet_size);

// Gets the packet size for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
size_t pirate_get_packet_size(int gd);

// Sets the packet count for the gaps channel buffer.
// Only valid if the channel type is SHMEM_UDP.
// If zero then SHMEM will allocate DEFAULT_PACKET_COUNT packet size.
// On error -1 is returned, and errno is set appropriately.
int pirate_set_packet_count(int gd, size_t packet_count);

// Gets the packet count for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
size_t pirate_get_packet_count(int gd);

// Sets the memory buffer size for the gaps channel.
// Only valid if the channel type is SHMEM or UNIX_SOCKET.
// If zero then SHMEM will allocate DEFAULT_SHMEM_BUFFER bytes.
// On error -1 is returned, and errno is set appropriately.
int pirate_set_buffer_size(int gd, int buffer_size);

// Gets the shared memory buffer size for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
int pirate_get_buffer_size(int gd);

// Sets the iovector length for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
int pirate_set_iov_length(int gd, size_t iov_len);

// Gets the iovector length for the gaps channel.
// On error -1 is returned, and errno is set appropriately.
ssize_t pirate_get_iov_length(int gd);

// Invoke fcntl() on the underlying file descriptor
int pirate_fcntl0(int gd, int flags, int cmd);
int pirate_fcntl1_int(int gd, int flags, int cmd, int arg);

// Invoke ioctl() on the underlying device
int pirate_ioctl0(int gd, int flags, long cmd);
int pirate_ioctl1_int(int gd, int flags, long cmd, int arg);

// Invoke getsockopt() on the underlying socket
int pirate_getsockopt(int gd, int flags, int level, int optname, void *optval,
                      socklen_t *optlen);

// Invoke setsockopt() on the underlying socket
int pirate_setsockopt(int gd, int flags, int level, int optname,
                      const void *optval, socklen_t optlen);

#ifdef __cplusplus
}
#endif

#endif //__PIRATE_PRIMITIVES_H
