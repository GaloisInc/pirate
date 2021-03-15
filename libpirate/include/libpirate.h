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
 * Copyright 2019-2020 Two Six Labs, LLC.  All rights reserved.
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
#define PIRATE_NUM_CHANNELS 32
#define PIRATE_IOV_MAX 16

#define PIRATE_DEFAULT_MIN_TX 512

typedef enum {
    // The gaps channel is unavailable for operations
    INVALID = 0,

    // The gaps channel is implemented using a filepath.
    // This filepath points to a character device, named pipe, or a derived
    // channel type.
    // Configuration parameters -#include <termios.h> pirate_device_param_t
    //  - path        - device path
    //  - min_tx_size - minimum transmit size (bytes)
    DEVICE,

    // The gaps channel is implemented using a FIFO special file
    // (a named pipe).
    // Configuration parameters - pirate_pipe_param_t
    //  - path        - file path to named pipe
    //  - min_tx_size - minimum transmit size (bytes)
    PIPE,

    // The gaps channel is implemented using a Unix domain socket.
    //  - path        - file path to unix socket
    //  - buffer_size - unix socket buffer size
    //  - min_tx_size - minimum transmit size (bytes)
    UNIX_SOCKET,

    // The gaps channel is implemented using a Unix domain socket with SOCK_SEQPACKET semantics.
    //  - path        - file path to unix socket
    //  - buffer_size - unix socket buffer size
    //  - min_tx_size - minimum transmit size (bytes)
    UNIX_SEQPACKET,

    // The gaps channel is implemented by using TCP sockets.
    // Configuration parameters - pirate_tcp_socket_param_t
    //  - reader_addr  - IP address on read end
    //  - reader_port  - IP port on read end
    //  - writer_addr  - IP address on write end (or 0.0.0.0)
    //  - writer_port  - IP port on write end (or 0)
    //  - buffer_size  - TCP socket buffer size
    //  - min_tx_size  - minimum transmit size (bytes)
    TCP_SOCKET,

    // The gaps channel is implemented by using UDP sockets.
    // Configuration parameters - pirate_udp_socket_param_t
    //  - reader_addr  - IP address on read end
    //  - reader_port  - IP port on read end
    //  - writer_addr  - IP address on write end (or 0.0.0.0)
    //  - writer_port  - IP port on write end (or 0)
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
    //  - Messages       - Message IDs, optional
    MERCURY,

    // The gaps channel for GRC Ethernet devices
    // Configuration parameters - pirate_ge_eth_param_t
    //  - reader_addr  - IP address on read end
    //  - reader_port  - IP port on read end
    //  - writer_addr  - IP address on write end (or 0.0.0.0)
    //  - writer_port  - IP port on write end (or 0)
    //  - message_id - send/receive message ID
    //  - mtu        - maximum frame length, default 1454
    GE_ETH,

   // Number of GAPS channel types
    PIRATE_CHANNEL_TYPE_COUNT
} channel_enum_t;

// DEVICE parameters
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned mtu;
    unsigned min_tx;
} pirate_device_param_t;

// PIPE parameters
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned mtu;
    unsigned min_tx;
} pirate_pipe_param_t;

// UNIX_SOCKET parameters
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
    unsigned mtu;
    unsigned min_tx;
} pirate_unix_socket_param_t;

// UNIX_SEQPACKET parameters
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
    unsigned mtu;
    unsigned min_tx;
} pirate_unix_seqpacket_param_t;

// TCP_SOCKET parameters
typedef struct {
    char reader_addr[INET6_ADDRSTRLEN];
    char writer_addr[INET6_ADDRSTRLEN];
    uint16_t reader_port;
    uint16_t writer_port;
    unsigned buffer_size;
    unsigned mtu;
    unsigned min_tx;
} pirate_tcp_socket_param_t;

// UDP_SOCKET parameters
#define PIRATE_DEFAULT_UDP_PACKET_SIZE             65535u
typedef struct {
    char reader_addr[INET6_ADDRSTRLEN];
    char writer_addr[INET6_ADDRSTRLEN];
    uint16_t reader_port;
    uint16_t writer_port;
    unsigned buffer_size;
    unsigned mtu;
} pirate_udp_socket_param_t;

// SHMEM parameters
#define PIRATE_DEFAULT_SMEM_BUF_LEN                (128u << 10)
#define PIRATE_DEFAULT_SMEM_MAX_TX                 65536u
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
    unsigned mtu;
    unsigned max_tx;
} pirate_shmem_param_t;

// UDP_SHMEM parameters
#define PIRATE_DEFAULT_UDP_SHMEM_PACKET_COUNT      1000u
#define PIRATE_DEFAULT_UDP_SHMEM_PACKET_SIZE       1024u
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned buffer_size;
    size_t packet_size;
    size_t packet_count;
    unsigned mtu;
} pirate_udp_shmem_param_t;

// UIO parameters
#define PIRATE_UIO_DEFAULT_PATH    "/dev/uio0"
#define PIRATE_UIO_DEFAULT_MAX_TX  65536u
typedef struct {
    char path[PIRATE_LEN_NAME];
    unsigned short region;
    unsigned mtu;
    unsigned max_tx;
} pirate_uio_param_t;

// SERIAL parameters
#define PIRATE_SERIAL_DEFAULT_BAUD     B230400
#define PIRATE_SERIAL_DEFAULT_MAX_TX   1024u
typedef struct {
    char path[PIRATE_LEN_NAME];
    speed_t baud;
    unsigned mtu;
    unsigned max_tx;
} pirate_serial_param_t;

typedef enum {
    MERCURY_UNKNOWN,
    MERCURY_IMMEDIATE,
    MERCURY_PAYLOAD
} mercury_mode_t;

// MERCURY parameters
#define PIRATE_MERCURY_ROOT_DEV             "/dev/gaps_ilip_0_root"
// The mercury mtu is needed while the
// read() API does not use host_payload_address
// pointer. If that is changed then remove the mtu.
#define PIRATE_MERCURY_DEFAULT_MTU          65536u
#define PIRATE_MERCURY_IMMEDIATE_SIZE       192u
#define PIRATE_MERCURY_DMA_DESCRIPTOR       256u
typedef struct {
    mercury_mode_t mode;
    uint32_t session_id;
    uint32_t message_id;
    uint32_t data_tag;
    uint32_t descriptor_tag;
    uint32_t mtu;
} pirate_mercury_param_t;

// GE_ETH parameters
#define PIRATE_DEFAULT_GE_ETH_MTU      1454u
typedef struct {
    char reader_addr[INET6_ADDRSTRLEN];
    char writer_addr[INET6_ADDRSTRLEN];
    uint16_t reader_port;
    uint16_t writer_port;
    uint32_t message_id;
    uint32_t mtu;
} pirate_ge_eth_param_t;

typedef struct {
    channel_enum_t channel_type;
    uint8_t drop;
    union {
        pirate_device_param_t           device;
        pirate_pipe_param_t             pipe;
        pirate_unix_socket_param_t      unix_socket;
        pirate_unix_seqpacket_param_t   unix_seqpacket;
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

typedef struct {
    uint64_t requests;
    uint64_t success;
    uint64_t errs; // EAGAIN and EWOULDBLOCK are not errors
    uint64_t fuzzed;
    uint64_t bytes; // bytes is incremented only on successful requests
} pirate_stats_t;

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

// Get channel parameters as a string
//
// Parameters
//  gd           - GAPS channel number
//  str          - string to contain channel description
//  len          - max len allowed in desc
//
// Upon successful return, this function returns the
// number of characters printed (excluding the null byte
// used to end output to strings). The function does not
// write more than len bytes (including the terminating null
// byte ('\0')). If the output was truncated due to this
// limit, then the return value is the number of characters
// (excluding the terminating null byte) which would have been
// written to the final string if enough space had been
// available. Thus, a return value of len or more means that
// the output was truncated. If an output error is encountered,
// a negative value is returned.

int pirate_unparse_channel_param(const pirate_channel_param_t *param, char *str, int len);

#define OPT_DELIM ","
#define KV_DELIM "="
#define GAPS_CHANNEL_OPTIONS                                                                   \
    "Supported channels:\n"                                                                    \
    "  DEVICE        device,path[,min_tx_size=N,mtu=N]\n"                                      \
    "  PIPE          pipe,path[,min_tx_size=N,mtu=N]\n"                                        \
    "  UNIX SOCKET   unix_socket,path[,buffer_size=N,min_tx_size=N,mtu=N]\n"                   \
    "  TCP SOCKET    tcp_socket,reader addr,reader port,writer addr,writer port[,buffer_size=N,min_tx_size=N,mtu=N]\n" \
    "  UDP SOCKET    udp_socket,reader addr,reader port,writer addr,writer port[,buffer_size=N,mtu=N]\n"               \
    "  SHMEM         shmem,path[,buffer_size=N,max_tx_size=N,mtu=N]\n"                         \
    "  UDP_SHMEM     udp_shmem,path[,buffer_size=N,packet_size=N,packet_count=N,mtu=N]\n"      \
    "  UIO           uio[,path=N,max_tx_size=N,mtu=N]\n"                                       \
    "  SERIAL        serial,path[,baud=N,max_tx_size=N,mtu=N]\n"                               \
    "  MERCURY       mercury,mode=[immediate|payload],session=N,message=N,data=N[,descriptor=N,mtu=N]\n"               \
    "  GE_ETH        ge_eth,reader addr,reader port,writer addr,writer port,msg_id[,mtu=N]\n"

// Copies channel parameters from configuration into param argument.
//
// Parameters
//  gd           - GAPS channel number
//  param        - channel-specific parameters
//
// Return:
//  0 on success
// -1 on failure, errno is set

int pirate_get_channel_param(int gd, pirate_channel_param_t *param);

// Get channel parameters as a string
//
// Parameters
//  gd           - GAPS channel number
//  str          - string to contain channel description
//  len          - max len allowed in desc
//
// Upon successful return, this function returns the
// number of characters printed (excluding the null byte
// used to end output to strings). The function does not
// write more than len bytes (including the terminating null
// byte ('\0')). If the output was truncated due to this
//  limit, then the return value is the number of characters
// (excluding the terminating null byte) which would have been
// written to the final string if enough space had been
// available. Thus, a return value of len or more means that
// the output was truncated. If an output error is encountered,
// a negative value is returned.

int pirate_get_channel_description(int gd, char *str, int len);

// Opens the gaps channel specified by parameter string.
//
// Channels must be opened in the same order across all
// processes.
//
// The argument flags must have access mode O_RDONLY or O_WRONLY.
//
// The return value is -1 if an error occurred (in which case,
// errno is set appropriately). Otherwise the return value represents
// a successful open.
//
// If the return value is a nonnegative integer then the gaps
// descriptor is also a valid unix file descriptor.
// If the return value is a negative integer less than -1 then
// the gaps descriptor is not a file descriptor.

int pirate_open_parse(const char *param, int flags);

// Opens the gaps channel specified by the parameter value.
//
// Channels must be opened in the same order across all
// processes.
//
// The argument flags must have access mode O_RDONLY or O_WRONLY.
//
// The return value is -1 if an error occurred (in which case,
// errno is set appropriately). Otherwise the return value represents
// a successful open.
//
// If the return value is a nonnegative integer then the gaps
// descriptor is also a valid unix file descriptor.
// If the return value is a negative integer less than -1 then
// the gaps descriptor is not a file descriptor.

int pirate_open_param(pirate_channel_param_t *param, int flags);

// Returns 1 if the channel type supports the
// O_NONBLOCK flag to pirate_open(). Otherwise return 0.

int pirate_nonblock_channel_type(channel_enum_t channel_type, size_t mtu);

// Returns a reference to the read and write statistics
// associated with the gaps descriptor.
//
// On success, the file descriptor is returned.
// On error NULL is returned, and errno is set appropriately.

const pirate_stats_t *pirate_get_stats(int gd);

// pirate_read() attempts to read the next packet of up
// to count bytes from gaps descriptor gd to the buffer
// starting at buf.
//
// If the packet is longer than count bytes, then you get the
// first size bytes of the packet and the rest of the packet
// is lost.
//
// On success, the number of bytes read is returned.
// On error, -1 is returned, and errno is set appropriately.

ssize_t pirate_read(int gd, void *buf, size_t count);

// pirate_write() writes the next packet of count bytes
// from the buffer starting at buf to the gaps descriptor
// gd.
//
// On success, the number of bytes written is returned.
// On error, -1 is returned, and errno is set appropriately.

ssize_t pirate_write(int gd, const void *buf, size_t count);

// pirate_write_mtu() returns the maximum data length
// that can be send in a call to pirate_write() for
// the given channel. A value of 0 indicates no maximum length.
//
// On success, the mtu is returned. On error,
// -1 is returned, and errno is set appropriately.

ssize_t pirate_write_mtu(int gd);

// Closes the gaps channel specified by the gaps descriptor.
//
// pirate_close() returns zero on success.  On error,
// -1 is returned, and errno is set appropriately.

int pirate_close(int gd);

#ifdef __cplusplus
}
#endif

#endif //__PIRATE_PRIMITIVES_H
