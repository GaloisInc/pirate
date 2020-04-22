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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic_int pirate_atomic_int;
#define ATOMIC_INC(PTR) std::atomic_fetch_add(PTR, 1)
#elif HAVE_STD_ATOMIC
#include <stdatomic.h>
typedef atomic_int pirate_atomic_int;
#define ATOMIC_INC(PTR) atomic_fetch_add(PTR, 1)
#else
typedef int pirate_atomic_int;
#define ATOMIC_INC(PTR) __atomic_fetch_add(PTR, 1, __ATOMIC_SEQ_CST)
#endif

#include "libpirate.h"
#include "device.h"
#include "pipe.h"
#include "unix_socket.h"
#include "tcp_socket.h"
#include "udp_socket.h"
#include "shmem_interface.h"
#include "udp_shmem_interface.h"
#include "uio.h"
#include "serial.h"
#include "mercury.h"
#include "ge_eth.h"
#include "pirate_common.h"

typedef struct {
    int flags;
    union {
        device_ctx         device;
        pipe_ctx           pipe;
        unix_socket_ctx    unix_socket;
        tcp_socket_ctx     tcp_socket;
        udp_socket_ctx     udp_socket;
        shmem_ctx          shmem;
        udp_shmem_ctx      udp_shmem;
        uio_ctx            uio;
        serial_ctx         serial;
        mercury_ctx        mercury;
        ge_eth_ctx         ge_eth;
    } channel;
} pirate_channel_ctx_t;

typedef struct {
    pirate_channel_param_t param;
    pirate_channel_ctx_t ctx;
} pirate_channel_t;

pirate_channel_t gaps_channels[PIRATE_NUM_CHANNELS];

int gaps_reader_gds[PIRATE_NUM_CHANNELS];
int gaps_reader_gds_num;

// TODO YIELD: convert this into an array
int gaps_writer_control_gd;

int pirate_close_channel(pirate_channel_t *channel);

static inline pirate_channel_t *pirate_get_channel(int gd) {
    pirate_channel_t *channel;
    if ((gd < 0) || (gd >= PIRATE_NUM_CHANNELS)) {
        errno = EBADF;
        return NULL;
    }

    channel = &gaps_channels[gd];
    if (channel->param.channel_type == INVALID) {
        errno = EBADF;
        return NULL;
    }

    return channel;
}

void pirate_init_channel_param(channel_enum_t channel_type, pirate_channel_param_t *param) {
    memset(param, 0, sizeof(*param));
    param->channel_type = channel_type;
}

static void pirate_parse_common_kv(const char *key, const char *value, pirate_channel_param_t *param) {
    if (strncmp("yield", key, strlen("yield")) == 0) {
        param->yield = atoi(value);
    } else if (strncmp("control", key, strlen("control")) == 0) {
        param->control = atoi(value);
    }
}

static void pirate_parse_common_param(char *str, pirate_channel_param_t *param) {
    char *token, *key, *value;
    char *saveptr1, *saveptr2;

    while ((token = strtok_r(str, OPT_DELIM, &saveptr1)) != NULL) {
        str = NULL;
        key = strtok_r(token, KV_DELIM, &saveptr2);
        if (key == NULL) {
            continue;
        }
        value = strtok_r(NULL, KV_DELIM, &saveptr2);
        if (value == NULL) {
            continue;
        }
        pirate_parse_common_kv(key, value, param);
    }
}

int pirate_parse_channel_param(const char *str, pirate_channel_param_t *param) {

    // Channel configuration function is allowed to modify the string
    // while braking it into delimiter-separated tokens
    char opt[256];
    strncpy(opt, str, sizeof(opt));

    pirate_init_channel_param(INVALID, param);

    pirate_parse_common_param(opt, param);

    strncpy(opt, str, sizeof(opt));

    if (strncmp("device", opt, strlen("device")) == 0) {
        param->channel_type = DEVICE;
        return pirate_device_parse_param(opt, &param->channel.device);
    } else if (strncmp("pipe", opt, strlen("pipe")) == 0) {
        param->channel_type = PIPE;
        return pirate_pipe_parse_param(opt, &param->channel.pipe);
    } else if (strncmp("unix_socket", opt, strlen("unix_socket")) == 0) {
        param->channel_type = UNIX_SOCKET;
        return pirate_unix_socket_parse_param(opt, &param->channel.unix_socket);
    } else if (strncmp("tcp_socket", opt, strlen("tcp_socket")) == 0) {
        param->channel_type = TCP_SOCKET;
        return pirate_tcp_socket_parse_param(opt, &param->channel.tcp_socket);
    } else if (strncmp("udp_socket", opt, strlen("udp_socket")) == 0) {
        param->channel_type = UDP_SOCKET;
        return pirate_udp_socket_parse_param(opt, &param->channel.udp_socket);
    } else if (strncmp("shmem", opt, strlen("shmem")) == 0) {
        param->channel_type = SHMEM;
        return pirate_shmem_parse_param(opt, &param->channel.shmem);
    } else if (strncmp("udp_shmem", opt, strlen("udp_shmem")) == 0) {
        param->channel_type = UDP_SHMEM;
        return pirate_udp_shmem_parse_param(opt, &param->channel.udp_shmem);
    } else if (strncmp("uio", opt, strlen("uio")) == 0) {
        param->channel_type = UIO_DEVICE;
        return pirate_uio_parse_param(opt, &param->channel.uio);
    } else if (strncmp("serial", opt, strlen("serial")) == 0) {
        param->channel_type = SERIAL;
        return pirate_serial_parse_param(opt, &param->channel.serial);
    } else if (strncmp("mercury", opt, strlen("mercury")) == 0) {
        param->channel_type = MERCURY;
        return pirate_mercury_parse_param(opt, &param->channel.mercury);
    } else if (strncmp("ge_eth", opt, strlen("ge_eth")) == 0) {
        param->channel_type = GE_ETH;
        return pirate_ge_eth_parse_param(opt, &param->channel.ge_eth);
    }

    errno = EINVAL;
    return -1;
}

int pirate_get_channel_param(int gd, pirate_channel_param_t *param) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }
    memcpy(param, &channel->param, sizeof(pirate_channel_param_t));
    return 0;
}

int pirate_get_channel_flags(int gd) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }
    return channel->ctx.flags;
}

pirate_channel_param_t *pirate_get_channel_param_ref(int gd) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return NULL;
    }

    return &channel->param;
}

int pirate_unparse_channel_param(const pirate_channel_param_t *param, char *desc, int len) {
    switch (param->channel_type) {
    case DEVICE:
        return pirate_device_get_channel_description(&param->channel.device, desc, len);

    case PIPE:
        return pirate_pipe_get_channel_description(&param->channel.pipe, desc, len);

    case UNIX_SOCKET:
        return pirate_unix_socket_get_channel_description(&param->channel.unix_socket, desc, len);

    case TCP_SOCKET:
        return pirate_tcp_socket_get_channel_description(&param->channel.tcp_socket, desc, len);

    case UDP_SOCKET:
        return pirate_udp_socket_get_channel_description(&param->channel.udp_socket, desc, len);

    case SHMEM:
        return pirate_shmem_get_channel_description(&param->channel.shmem, desc, len);

    case UDP_SHMEM:
        return pirate_udp_shmem_get_channel_description(&param->channel.udp_shmem, desc, len);

    case UIO_DEVICE:
        return pirate_uio_get_channel_description(&param->channel.uio, desc, len);

    case SERIAL:
        return pirate_serial_get_channel_description(&param->channel.serial, desc, len);

    case MERCURY:
        return pirate_mercury_get_channel_description(&param->channel.mercury, desc, len);

    case GE_ETH:
        return pirate_ge_eth_get_channel_description(&param->channel.ge_eth, desc, len);

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }
}

int pirate_get_channel_description(int gd, char *desc, int len) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    return pirate_unparse_channel_param(&channel->param, desc, len);
}

static pirate_atomic_int next_gd;

static int pirate_next_gd() {
    int next = ATOMIC_INC(&next_gd);
    if (next >= PIRATE_NUM_CHANNELS) {
        return -1;
    }
    return next;
}

// Declared in libpirate_internal.h for testing purposes only
void pirate_reset_gd() {
    next_gd = 0;
}

static int pirate_open(pirate_channel_t *channel, int flags) {
    pirate_channel_param_t *param = &channel->param;
    pirate_channel_ctx_t *ctx = &channel->ctx;
    int access = flags & O_ACCMODE;

    if ((access != O_RDONLY) && (access != O_WRONLY)) {
        errno = EINVAL;
        return -1;
    }

    switch (param->channel_type) {
    case DEVICE:
        return pirate_device_open(flags, &param->channel.device, &ctx->channel.device);

    case PIPE:
        return pirate_pipe_open(flags, &param->channel.pipe, &ctx->channel.pipe);

    case UNIX_SOCKET:
        return pirate_unix_socket_open(flags, &param->channel.unix_socket, &ctx->channel.unix_socket);

    case TCP_SOCKET:
        return pirate_tcp_socket_open(flags, &param->channel.tcp_socket, &ctx->channel.tcp_socket);

    case UDP_SOCKET:
        return pirate_udp_socket_open(flags, &param->channel.udp_socket, &ctx->channel.udp_socket);

    case SHMEM:
        return pirate_shmem_open(flags, &param->channel.shmem, &ctx->channel.shmem);

    case UDP_SHMEM:
        return pirate_udp_shmem_open(flags, &param->channel.udp_shmem, &ctx->channel.udp_shmem);

    case UIO_DEVICE:
        return pirate_uio_open(flags, &param->channel.uio, &ctx->channel.uio);

    case SERIAL:
        return pirate_serial_open(flags, &param->channel.serial, &ctx->channel.serial);

    case MERCURY:
        return pirate_mercury_open(flags, &param->channel.mercury, &ctx->channel.mercury);

    case GE_ETH:
        return pirate_ge_eth_open(flags, &param->channel.ge_eth, &ctx->channel.ge_eth);

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }
}

static void pirate_yield_setup(int gd, pirate_channel_param_t *param, int access) {
    if ((access == O_RDONLY) && (param->yield || param->control)) {
        gaps_reader_gds[gaps_reader_gds_num++] = gd;
    }
    if ((access == O_WRONLY) && param->control) {
        gaps_writer_control_gd = gd;
    }
}

// gaps descriptors must be opened from smallest to largest
int pirate_open_param(pirate_channel_param_t *param, int flags) {
    pirate_channel_t channel;
    int access = flags & O_ACCMODE;

    if (next_gd >= PIRATE_NUM_CHANNELS) {
        errno = EMFILE;
        return -1;
    }

    memcpy(&channel.param, param, sizeof(pirate_channel_param_t));
    channel.ctx.flags = flags;

    if (pirate_open(&channel, flags) < 0) {
        return -1;
    }

    int gd = pirate_next_gd();
    if (gd < 0) {
        pirate_close_channel(&channel);
        errno = EMFILE;
        return -1;
    }

    memcpy(&gaps_channels[gd], &channel, sizeof(pirate_channel_t));
    pirate_yield_setup(gd, param, access);
    return gd;
}

int pirate_open_parse(const char *param, int flags) {
    pirate_channel_param_t vals;

    if (pirate_parse_channel_param(param, &vals) < 0) {
        return -1;
    }

    return pirate_open_param(&vals, flags);
}

int pirate_pipe_channel_type(channel_enum_t channel_type) {
    switch (channel_type) {
    case PIPE:
        return 1;
    default:
        return 0;
    }
}

int pirate_pipe_param(int gd[2], pirate_channel_param_t *param, int flags) {
    pirate_channel_t read_channel, write_channel;
    int rv, read_gd, write_gd;
    int access = flags & O_ACCMODE;
    int behavior = flags & ~O_ACCMODE;

    if (!pirate_pipe_channel_type(param->channel_type)) {
        errno = ENOSYS;
        return -1;
    }

    if (next_gd >= (PIRATE_NUM_CHANNELS - 1)) {
        errno = EMFILE;
        return -1;
    }

    if (access != O_RDWR) {
        errno = EINVAL;
        return -1;
    }

    param->pipe = 1; // TODO YIELD: remove and rely on src and dst enclave id
    memcpy(&read_channel.param, param, sizeof(pirate_channel_param_t));
    memcpy(&write_channel.param, param, sizeof(pirate_channel_param_t));
    read_channel.ctx.flags = behavior | O_RDONLY;
    write_channel.ctx.flags = behavior | O_WRONLY;

    switch (param->channel_type) {
    case PIPE:
        rv = pirate_pipe_pipe(flags, &param->channel.pipe,
            &read_channel.ctx.channel.pipe,
            &write_channel.ctx.channel.pipe);
        break;
    default:
        errno = ENOSYS;
        rv = -1;
    }

    if (rv < 0) {
        return rv;
    }

    read_gd = pirate_next_gd();
    write_gd = pirate_next_gd();
    if ((read_gd < 0) || (write_gd < 0)) {
        pirate_close_channel(&read_channel);
        pirate_close_channel(&write_channel);
        errno = EMFILE;
        return -1;
    }

    memcpy(&gaps_channels[read_gd], &read_channel, sizeof(pirate_channel_t));
    memcpy(&gaps_channels[write_gd], &write_channel, sizeof(pirate_channel_t));
    pirate_yield_setup(read_gd, param, O_RDONLY);
    pirate_yield_setup(write_gd, param, O_WRONLY);

    gd[0] = read_gd;
    gd[1] = write_gd;
    return 0;
}

int pirate_pipe_parse(int gd[2], const char *param, int flags) {
    pirate_channel_param_t vals;

    if (pirate_parse_channel_param(param, &vals) < 0) {
        return -1;
    }

    return pirate_pipe_param(gd, &vals, flags);
}

int pirate_get_fd(int gd) {
    pirate_channel_t *channel;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    switch (channel->param.channel_type) {
    case DEVICE:
        return channel->ctx.channel.device.fd;
    case PIPE:
        return channel->ctx.channel.pipe.fd;
    case UNIX_SOCKET:
        return channel->ctx.channel.unix_socket.sock;
    case TCP_SOCKET:
        return channel->ctx.channel.tcp_socket.sock;
    case UDP_SOCKET:
        return channel->ctx.channel.udp_socket.sock;
    case SERIAL:
        return channel->ctx.channel.serial.fd;
    case MERCURY:
        return channel->ctx.channel.mercury.fd;
    case GE_ETH:
        return channel->ctx.channel.ge_eth.sock;
    default:
        errno = ENODEV;
        return -1;
    }
}

int pirate_close(int gd) {
    pirate_channel_t *channel;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }
    return pirate_close_channel(channel);
}

int pirate_close_channel(pirate_channel_t *channel) {
    int rv;
    pirate_channel_ctx_t *ctx = &channel->ctx;

    switch (channel->param.channel_type) {

    case DEVICE:
        rv = pirate_device_close(&ctx->channel.device);
        break;
    case PIPE:
        rv = pirate_pipe_close(&ctx->channel.pipe);
        break;
    case UNIX_SOCKET:
        rv = pirate_unix_socket_close(&ctx->channel.unix_socket);
        break;
    case TCP_SOCKET:
        rv = pirate_tcp_socket_close(&ctx->channel.tcp_socket);
        break;
    case UDP_SOCKET:
        rv = pirate_udp_socket_close(&ctx->channel.udp_socket);
        break;
    case SHMEM:
        rv = pirate_shmem_close(&ctx->channel.shmem);
        break;
    case UDP_SHMEM:
        rv = pirate_udp_shmem_close(&ctx->channel.udp_shmem);
        break;
    case UIO_DEVICE:
        rv = pirate_uio_close(&ctx->channel.uio);
        break;
    case SERIAL:
        rv = pirate_serial_close(&ctx->channel.serial);
        break;
    case MERCURY:
        rv = pirate_mercury_close(&ctx->channel.mercury);
        break;
    case GE_ETH:
        rv = pirate_ge_eth_close(&ctx->channel.ge_eth);
        break;
    case INVALID:
    default:
        errno = ENODEV;
        rv = -1;
        break;
    }
    if (rv < 0) {
        return rv;
    }
    channel->param.channel_type = INVALID;
    return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.flags & O_ACCMODE) != O_RDONLY) {
        errno = EBADF;
        return -1;
    }

    pirate_channel_param_t *param = &channel->param;
    pirate_channel_ctx_t *ctx = &channel->ctx;

    switch (channel->param.channel_type) {

    case DEVICE:
        return pirate_device_read(&param->channel.device, &ctx->channel.device, buf, count);

    case PIPE:
        return pirate_pipe_read(&param->channel.pipe, &ctx->channel.pipe, buf, count);

    case UNIX_SOCKET:
        return pirate_unix_socket_read(&param->channel.unix_socket, &ctx->channel.unix_socket, buf, count);

    case TCP_SOCKET:
        return pirate_tcp_socket_read(&param->channel.tcp_socket, &ctx->channel.tcp_socket, buf, count);

    case UDP_SOCKET:
        return pirate_udp_socket_read(&param->channel.udp_socket, &ctx->channel.udp_socket, buf, count);

    case SHMEM:
        return pirate_shmem_read(&param->channel.shmem, &ctx->channel.shmem, buf, count);

    case UDP_SHMEM:
        return pirate_udp_shmem_read(&param->channel.udp_shmem, &ctx->channel.udp_shmem, buf, count);

    case UIO_DEVICE:
        return pirate_uio_read(&param->channel.uio, &ctx->channel.uio, buf, count);

    case SERIAL:
        return pirate_serial_read(&param->channel.serial, &ctx->channel.serial, buf, count);

    case MERCURY:
        return pirate_mercury_read(&param->channel.mercury, &ctx->channel.mercury, buf, count);

    case GE_ETH:
        return pirate_ge_eth_read(&param->channel.ge_eth, &ctx->channel.ge_eth, buf, count);

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
    pirate_channel_t *channel = NULL;
    ssize_t rv;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.flags & O_ACCMODE) != O_WRONLY) {
        errno = EBADF;
        return -1;
    }

    pirate_channel_param_t *param = &channel->param;
    pirate_channel_ctx_t *ctx = &channel->ctx;

    switch (param->channel_type) {

    case DEVICE:
        rv = pirate_device_write(&param->channel.device, &ctx->channel.device, buf, count);
        break;

    case PIPE:
        rv = pirate_pipe_write(&param->channel.pipe, &ctx->channel.pipe, buf, count);
        break;

    case UNIX_SOCKET:
        rv = pirate_unix_socket_write(&param->channel.unix_socket, &ctx->channel.unix_socket, buf, count);
        break;

    case TCP_SOCKET:
        rv = pirate_tcp_socket_write(&param->channel.tcp_socket, &ctx->channel.tcp_socket, buf, count);
        break;

    case UDP_SOCKET:
        rv = pirate_udp_socket_write(&param->channel.udp_socket, &ctx->channel.udp_socket, buf, count);
        break;

    case SHMEM:
        rv = pirate_shmem_write(&param->channel.shmem, &ctx->channel.shmem, buf, count);
        break;

    case UDP_SHMEM:
        rv = pirate_udp_shmem_write(&param->channel.udp_shmem, &ctx->channel.udp_shmem, buf, count);
        break;

    case UIO_DEVICE:
        rv = pirate_uio_write(&param->channel.uio, &ctx->channel.uio, buf, count);
        break;

    case SERIAL:
        rv = pirate_serial_write(&param->channel.serial, &ctx->channel.serial, buf, count);
        break;

    case MERCURY:
        rv = pirate_mercury_write(&param->channel.mercury, &ctx->channel.mercury, buf, count);
        break;

    case GE_ETH:
        rv = pirate_ge_eth_write(&param->channel.ge_eth, &ctx->channel.ge_eth, buf, count);
        break;

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }

    if (rv < 0) {
        return rv;
    }
    if ((rv > 0) && param->yield && !param->control) {
        int ret = pirate_listen();
        if (ret < 0) {
            return ret;
        }
    }
    return rv;
}
