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

#include <BaseTsd.h>
#include <stdint.h>

#include <WinSock2.h>
#include <Ws2ipdef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIRATE_LEN_NAME 64
#define PIRATE_NUM_ENCLAVES 16
#define PIRATE_NUM_CHANNELS 16
#define PIRATE_IOV_MAX 16

#define PIRATE_DEFAULT_MIN_TX 512

typedef enum {
    // The gaps channel is unavailable for operations
    INVALID = 0,

    // The gaps channel is implemented by using TCP sockets.
    // Configuration parameters - pirate_tcp_socket_param_t
    //  - addr        - IP address, if empty then 127.0.0.1 is used
    //  - port        - IP port
    //  - buffer_size - UDP socket buffer size
    UDP_SOCKET,

    // The gaps channel for GRC Ethernet devices
    // Configuration parameters - pirate_ge_eth_param_t
    //  - addr       - IP address, if empty then 127.0.0.1 is used
    //  - port       - IP port
    //  - message_id - send/receive message ID
    //  - mtu        - maximum frame length, default 1454
    GE_ETH,

   // Number of GAPS channel types
    PIRATE_CHANNEL_TYPE_COUNT
} channel_enum_t;

// UDP_SOCKET parameters
#define PIRATE_DEFAULT_UDP_IP_ADDR                 "127.0.0.1"
#define PIRATE_DEFAULT_UDP_PACKET_SIZE             65535u
typedef struct {
    char addr[INET_ADDRSTRLEN];
    short port;
    unsigned buffer_size;
    unsigned mtu;
} pirate_udp_socket_param_t;

// GE_ETH parameters
#define PIRATE_DEFAULT_GE_ETH_IP_ADDR  "127.0.0.1"
#define PIRATE_DEFAULT_GE_ETH_MTU      1454u
typedef struct {
    char addr[INET_ADDRSTRLEN];
    short port;
    uint32_t message_id;
    uint32_t mtu;
} pirate_ge_eth_param_t;

typedef struct {
    channel_enum_t channel_type;
    union {
        pirate_udp_socket_param_t       udp_socket;
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
    "  UDP SOCKET    udp_socket,reader addr,reader port[,buffer_size=N,mtu=N]\n"               \
    "  GE_ETH        ge_eth,reader addr,reader port,msg_id[,mtu=N]\n"

// Copies channel parameters from configuration into param argument.
//
// Parameters
//  gd           - GAPS channel number
//  param        - channel-specific parameters
//
// Return:
//  0 on success
// -1 on failure, GetLastError() is set

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

// The return value is a unique gaps descriptor, or -1 if an
// error occurred (in which case, GetLastError() is set appropriately).
//
// The argument flags must have access mode O_RDONLY or O_WRONLY.

int pirate_open_parse(const char *param, int flags);

// Opens the gaps channel specified by the parameter value.
//
// Channels must be opened in the same order across all
// processes.

// The return value is a unique gaps descriptor, or -1 if an
// error occurred (in which case, GetLastError() is set appropriately).
//
// The argument flags must have access mode O_RDONLY or O_WRONLY.

int pirate_open_param(pirate_channel_param_t *param, int flags);

// Returns 1 if the channel type supports the
// pirate_pipe_param() and pirate_pipe_parse()
// functions. Otherwise return 0.

int pirate_pipe_channel_type(channel_enum_t channel_type);

// Returns 1 if the channel type supports the
// O_NONBLOCK flag to pirate_open(). Otherwise return 0.

int pirate_nonblock_channel_type(channel_enum_t channel_type, size_t mtu);

// Opens both ends of the gaps channel specified by the
// parameter value. See pipe() system call. Some channel
// types cannot be opened for both reading and writing.
//
// The caller is responsible for closing the reader and the writer.
//
// On success, zero is returned. On error, -1 is returned,
// and GetLastError() is set appropriately. If the channel type
// does not support this functionality then GetLastError() is set
// to ENOSYS.
//
// The argument flags must have access mode O_RDWR.

int pirate_pipe_param(int gd[2], pirate_channel_param_t *param, int flags);

// Opens both ends of the gaps channel specified by the
// parameter string. See pipe() system call. Some channel
// types cannot be opened for both reading and writing.
//
// The caller is responsible for closing the reader and the writer.
//
// On success, zero is returned. On error, -1 is returned,
// and GetLastError() is set appropriately. If the channel type
// does not support this functionality then GetLastError() is set
// to ENOSYS.
//
// The argument flags must have access mode O_RDWR.

int pirate_pipe_parse(int gd[2], const char *param, int flags);

// pirate_read() attempts to read the next packet of up
// to count bytes from gaps descriptor gd to the buffer
// starting at buf.
//
// If the packet is longer than count bytes, then you get the
// first size bytes of the packet and the rest of the packet
// is lost.
//
// On success, the number of bytes read is returned.
// On error, -1 is returned, and GetLastError() is set appropriately.

SSIZE_T pirate_read(int gd, void *buf, size_t count);

// pirate_write() writes the next packet of count bytes
// from the buffer starting at buf to the gaps descriptor
// gd.
//
// On success, the number of bytes written is returned. On error,
// -1 is returned, and GetLastError() is set appropriately.

SSIZE_T pirate_write(int gd, const void *buf, size_t count);


// pirate_write_mtu() returns the maximum data length
// that can be send in a call to pirate_write() for
// the given channel parameters. A value of 0 indicates no maximum length.
//
// On success, the mtu is returned. On error,
// -1 is returned, and GetLastError() is set appropriately.

SSIZE_T pirate_write_mtu(const pirate_channel_param_t *param);

// Closes the gaps channel specified by the gaps descriptor.
//
// pirate_close() returns zero on success.  On error,
// -1 is returned, and GetLastError() is set appropriately.

int pirate_close(int gd);

#ifdef __cplusplus
}
#endif

#endif //__PIRATE_PRIMITIVES_H
