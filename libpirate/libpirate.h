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
#include <stdint.h>
#include <termios.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIRATE_LEN_NAME 64
#define PIRATE_NUM_CHANNELS 16
#define PIRATE_IOV_MAX 16

typedef enum {
    // The gaps channel is unavailable for operations
    INVALID = 0,

    // The gaps channel is implemented using a filepath.
    // This filepath points to a character device, named pipe, or a derived
    // channel type.
    // Configuration parameters -#include <termios.h> pirate_device_param_t
    //  - path    - device path
    //  - iov_len - I/O vector chunk length
    DEVICE,

    // The gaps channel is implemented using a FIFO special file
    // (a named pipe).
    // Configuration parameters - pirate_pipe_param_t
    //  - path    - if "" then PIRATE_FILENAME format is used instead
    //  - iov_len - I/O vector chunk length
    PIPE,

    // The name of the socket is formatted with PIRATE_DOMAIN_FILENAME
    // where %d is the gaps descriptor.
    //  - path        - if "" then PIRATE_FILENAME format is used instead
    //  - iov_len     - I/O vector chunk length
    //  - buffer_size - unix socket buffer size
    UNIX_SOCKET,

    // The gaps channel is implemented by using TCP sockets.
    // Configuration parameters - pirate_tcp_socket_param_t
    //  - addr        - IP address, if empty then 127.0.0.1 is used
    //  - port        - IP port, if empty, then 26427 + aps descriptor is used
    //  - iov_len     - I/O vector chunk length
    //  - buffer_size - TCP socket buffer size
    TCP_SOCKET,

    // The gaps channel is implemented by using TCP sockets.
    // Configuration parameters - pirate_tcp_socket_param_t
    //  - addr        - IP address, if empty then 127.0.0.1 is used
    //  - port        - IP port, if empty, then 26427 + aps descriptor is used
    //  - iov_len     - I/O vector chunk length
    //  - buffer_size - UDP socket buffer size
    UDP_SOCKET,

    // The gaps channel is implemented using shared memory.
    // This feature is disabled by default. It must be enabled
    // by setting PIRATE_SHMEM_FEATURE to ON
    // Configuration parameters - pirate_shmem_param_t
    //  - path        - location of the shared memory
    //  - buffer_size - shared memory buffer size
    SHMEM,

    // The gaps channel is implemented using UDP packets
    // transmitted over shared memory. For measuring the
    // cost of creating UDP packets in userspace.
    // This feature is disabled by default. It must be enabled
    // by setting PIRATE_SHMEM_FEATURE to ON
    // Configuration parameters - pirate_udp_shmem_param_t
    //  - path         - location of the shared memory
    //  - buffer_size  - shared memory buffer size
    //  - packet_size  - optional packet size
    //  - packet_count - optional packet count
    UDP_SHMEM,

    // The gaps channel is implemented using userspace io.
    // The gaps uio device driver must be loaded.
    // Configuration parameters - pirate_uio_param_t
    //  - path         - device path
    UIO_DEVICE,

    // The gaps channel is implemented over a /dev/tty* interface
    // Default baud 30400, no error detection or correction
    // Configuration parameters - pirate_serial_param_t
    //  - path - device path
    //  - buad - baud rate, default 230400
    //  - mtu  - max transmit chunk, 1024
    SERIAL,

    // The gaps channel for Mercury System PCI-E device
    // Configuration parameters - pirate_mercury_param_t
    //  - Level          - Sensitivity level, required
    //  - Source ID      - Source ID, required
    //  - Destination ID - Destination ID, required
    //  - Timeout (ms)   - Read/write timeout in milliseconds, optional
    //  - Messages       - Message IDs, optional
    MERCURY,

    // The gaps channel for GRC Ethernet devices
    // Configuration parameters - pirate_ge_eth_param_t
    //  - addr - IP address, if empty then 127.0.0.1 is used
    //  - port - IP port, if empty, then 0x4745 + aps descriptor is used
    //  - mtu  - maximum frame length, default 1454
    GE_ETH
} channel_enum_t;

// DEVICE parameters
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned iov_len;
} pirate_device_param_t;

// PIPE parameters
#define PIRATE_PIPE_NAME_FMT                "/tmp/gaps.channel.%d"
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned iov_len;
} pirate_pipe_param_t;

// UNIX_SOCKET parameters
#define PIRATE_UNIX_SOCKET_NAME_FMT         "/tmp/gaps.channel.%d.sock"
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned iov_len;
    unsigned buffer_size;
} pirate_unix_socket_param_t;

// TCP_SOCKET parameters
#define DEFAULT_TCP_IP_ADDR                 "127.0.0.1"
#define PIRATE_TCP_PORT_BASE                26427
typedef struct {
    char addr[INET_ADDRSTRLEN];
    short port;
    unsigned iov_len;
    unsigned buffer_size;
} pirate_tcp_socket_param_t;

// UDP_SOCKET parameters
#define DEFAULT_UDP_IP_ADDR                 "127.0.0.1"
#define PIRATE_UDP_PORT_BASE                26427
typedef struct {
    char addr[INET_ADDRSTRLEN];
    short port;
    unsigned iov_len;
    unsigned buffer_size;
} pirate_udp_socket_param_t;

// SHMEM parameters
#define DEFAULT_SMEM_BUF_LEN                (128u << 10)
#define PIRATE_SHMEM_NAME_FMT               "/gaps.channel.%d"
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
} pirate_shmem_param_t;

// UDP_SHMEM parameters
#define DEFAULT_UDP_SHMEM_PACKET_COUNT      1000
#define DEFAULT_UDP_SHMEM_PACKET_SIZE       1024
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
    size_t packet_size;
    size_t packet_count;
} pirate_udp_shmem_param_t;

// UIO parameters
#define DEFAULT_UIO_DEVICE  "/dev/uio0"
typedef struct {
    char path[PIRATE_LEN_NAME];
} pirate_uio_param_t;

// SERIAL parameters
#define PIRATE_SERIAL_NAME_FMT  "/dev/ttyUSB%d"
#define SERIAL_DEFAULT_BAUD     B230400
#define SERIAL_DEFAULT_MTU      1024u
typedef struct {
    char path[PIRATE_LEN_NAME];
    speed_t baud;
    unsigned mtu;
} pirate_serial_param_t;

// MERCURY parameters
#define PIRATE_MERCURY_ROOT_DEV             "/dev/gaps_ilip_0_root"
#define PIRATE_MERCURY_DEFAULT_MTU          256u
#define PIRATE_MERCURY_DEFAULT_TIMEOUT_MS   1000u
#define PIRATE_MERCURY_MESSAGE_TABLE_LEN    16u
typedef struct {
    struct {
        uint32_t level;
        uint32_t source_id;
        uint32_t destination_id;
        uint32_t message_count;
        uint32_t messages[PIRATE_MERCURY_MESSAGE_TABLE_LEN];
        uint32_t id;
    } session;

    uint32_t mtu;
    uint32_t timeout_ms;
} pirate_mercury_param_t;

// GE_ETH parameters
#define DEFAULT_GE_ETH_IP_ADDR  "127.0.0.1"
#define DEFAULT_GE_ETH_IP_PORT  0x4745
#define DEFAULT_GE_ETH_MTU      1454u
typedef struct {
    char addr[INET_ADDRSTRLEN];
    short port;
    unsigned mtu;
} pirate_ge_eth_param_t;

typedef struct {
    channel_enum_t channel_type;
    union {
        pirate_device_param_t           device;
        pirate_pipe_param_t             pipe;
        pirate_unix_socket_param_t      unix_socket;
        pirate_tcp_socket_param_t       tcp_socket;
        pirate_udp_socket_param_t       udp_socket;
        pirate_shmem_param_t            shmem;
        pirate_udp_shmem_param_t        udp_shmem;
        pirate_uio_param_t              uio;
        pirate_serial_param_t           serial;
        pirate_mercury_param_t          mercury;
        pirate_ge_eth_param_t           ge_eth;
    } channel;
} pirate_channel_param_t;

//
// API
//

// Sets channel properties to the default values.
// The default value is represented by the zero value.
// Parameters:
//  channel_type - GAPS channel type
//  param        - channel parameters to be initialized
void pirate_init_channel_param(channel_enum_t channel_type, pirate_channel_param_t *param);

// Parse a string with gaps channel configuration options.
// Any parameters not specified will be set to the default value.
// The default value is represented by the zero value.
// Parameters:
//  str   - gaps channel configuration string
//  param - structure with configuration parameters
// Return:
//  0 on success
// -1 on failure
int pirate_parse_channel_param(const char *str, pirate_channel_param_t *param);

#define OPT_DELIM ","
#define GAPS_CHANNEL_OPTIONS                                                   \
    "Supported channels:\n"                                                    \
    "  DEVICE        device,path[,iov_len]\n"                                  \
    "  PIPE          pipe[,path,iov_len]\n"                                    \
    "  UNIX SOCKET   unix_socket[,path,iov_len,buffer_size]\n"                 \
    "  TCP SOCKET    tcp_socket[,addr,port,iov_len,buffer_size]\n"             \
    "  UDP SOCKET    udp_socket[,addr,port,iov_len,buffer_size]\n"             \
    "  SHMEM         shmem[,path,buffer_size]\n"                               \
    "  UDP_SHMEM     udp_shmem[,path,buffer_size,packet_size,packet_count]\n"  \
    "  UIO           uio[,path]\n"                                             \
    "  SERIAL        serial[,path,baud,mtu]\n"                                 \
    "  MERCURY       mercury,level,src_id,dst_id[,timeout_ms,msg_id_1,...]\n"  \
    "  GE_ETH        ge_eth[,addr,port,mtu]\n"

// Copies channel parameters from param argument into configuration.
//
// Parameters:
//  gd           - GAPS channel number
//  flags        - O_RDONLY or O_WRONLY
//  param        - channel-specific parameters.
// Return:
//  0 on success
// -1 on failure, errno is set

int pirate_set_channel_param(int gd, int flags, const pirate_channel_param_t *param);

// Copies channel parameters from configuration into param argument.
//
// Parameters
//  gd           - GAPS channel number
//  flags        - O_RDONLY or O_WRONLY
//  param        - channel-specific parameters
//
// Return:
//  0 on success
// -1 on failure, errno is set

int pirate_get_channel_param(int gd, int flags, pirate_channel_param_t *param);

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

#ifdef __cplusplus
}
#endif

#endif //__PIRATE_PRIMITIVES_H
