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

#include "primitives.h"
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
    channel_t type;

    union {
        pirate_device_ctx_t         device;
        pirate_pipe_ctx_t           pipe;
        pirate_unix_socket_ctx_t    unix_socket;
        pirate_tcp_socket_ctx_t     tcp_socket;
        pirate_udp_socket_ctx_t     udp_socket;
        pirate_shmem_ctx_t          shmem;
        pirate_udp_shmem_ctx_t      udp_shmem;
        pirate_uio_ctx_t            uio;
        pirate_serial_ctx_t         serial;
        pirate_mercury_ctx_t        mercury;
        pirate_ge_eth_ctx_t         ge_eth;
    };
} pirate_channel_ctx_t;

static struct {
    pirate_channel_ctx_t reader;
    pirate_channel_ctx_t writer;
} gaps_channels[PIRATE_NUM_CHANNELS];

static inline pirate_channel_ctx_t *pirate_get_channel(int gd, int flags) {
    if ((gd < 0) || (gd >= PIRATE_NUM_CHANNELS)) {
        errno = EBADF;
        return NULL;
    }

    if (flags == O_RDONLY) {
        return &gaps_channels[gd].reader;
    } else if (flags == O_WRONLY) {
        return &gaps_channels[gd].writer;
    }

    errno = EINVAL;
    return NULL;
}


int pirate_init_channel_param(channel_t channel_type, int gd, int flags,
    pirate_channel_param_t *param) {

    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = ENODEV;
        return -1;
    }

    if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
        errno = EINVAL;
        return -1;
    }

    switch (channel_type) {

    case DEVICE:
        return pirate_device_init_param(gd, flags, &param->device);

    case PIPE:
        return pirate_pipe_init_param(gd, flags, &param->pipe);

    case UNIX_SOCKET:
        return pirate_unix_socket_init_param(gd, flags, &param->unix_socket);

    case TCP_SOCKET:
        return pirate_tcp_socket_init_param(gd, flags, &param->tcp_socket);

    case UDP_SOCKET:
        return pirate_udp_socket_init_param(gd, flags, &param->udp_socket);

    case SHMEM:
        return pirate_shmem_init_param(gd, flags, &param->shmem);

    case UDP_SHMEM:
        return pirate_udp_shmem_init_param(gd, flags, &param->udp_shmem);

    case UIO_DEVICE:
        return pirate_uio_init_param(gd, flags, &param->uio);

    case SERIAL:
        return pirate_serial_init_param(gd, flags, &param->serial);

    case MERCURY:
        return pirate_mercury_init_param(gd, flags, &param->mercury);

    case GE_ETH:
        return pirate_ge_eth_init_param(gd, flags, &param->ge_eth);

    case INVALID:
    default:
        memset(param, 0, sizeof(*param));
        errno = EINVAL;
        break;
    }

    return -1;
}

channel_t pirate_parse_channel_param(int gd, int flags, const char *str,
                                        pirate_channel_param_t *param) {
    (void) gd, (void) flags;

    // Channel configuration function is allowed to modify the string
    // while braking it into delimiter-separated tokens
    char opt[256];
    strncpy(opt, str, sizeof(opt));

    if (strncmp("device", opt, strlen("device")) == 0) {
        if (pirate_device_parse_param(gd, flags, opt, &param->device) == 0) {
            return DEVICE;
        }
    } else if (strncmp("pipe", opt, strlen("pipe")) == 0) {
        if (pirate_pipe_parse_param(gd, flags, opt, &param->pipe) == 0) {
            return PIPE;
        }
    } else if (strncmp("unix_socket", opt, strlen("unix_socket")) == 0) {
        if (pirate_unix_socket_parse_param(gd, flags, opt,
                                            &param->unix_socket) == 0) {
            return UNIX_SOCKET;
        }
    } else if (strncmp("tcp_socket", opt, strlen("tcp_socket")) == 0) {
        if (pirate_tcp_socket_parse_param(gd, flags, opt,
                                            &param->tcp_socket) == 0) {
            return TCP_SOCKET;
        }
    } else if (strncmp("udp_socket", opt, strlen("udp_socket")) == 0) {
        if (pirate_udp_socket_parse_param(gd, flags, opt,
                                            &param->udp_socket) == 0) {
            return UDP_SOCKET;
        }
    } else if (strncmp("shmem", opt, strlen("shmem")) == 0) {
        if (pirate_shmem_parse_param(gd, flags, opt, &param->shmem) == 0) {
            return SHMEM;
        }
    } else if (strncmp("udp_shmem", opt, strlen("udp_shmem")) == 0) {
        if (pirate_udp_shmem_parse_param(gd, flags, opt,
                                            &param->udp_shmem) == 0) {
            return UDP_SHMEM;
        }
    } else if (strncmp("uio", opt, strlen("uio")) == 0) {
        if (pirate_uio_parse_param(gd, flags, opt, &param->uio) == 0) {
            return UIO_DEVICE;
        }
    } else if (strncmp("serial", opt, strlen("serial")) == 0) {
        if (pirate_serial_parse_param(gd, flags, opt, &param->serial) == 0) {
            return SERIAL;
        }
    } else if (strncmp("mercury", opt, strlen("mercury")) == 0) {
        if (pirate_mercury_parse_param(gd, flags, opt, &param->mercury) == 0) {
            return MERCURY;
        }
    } else if (strncmp("ge_eth", opt, strlen("ge_eth")) == 0) {
        if (pirate_ge_eth_parse_param(gd, flags, opt, &param->ge_eth) == 0) {
            return GE_ETH;
        }
    }

    return INVALID;
}

int pirate_set_channel_param(channel_t channel_type, int gd, int flags,
                                const pirate_channel_param_t *param) {
    int rv = -1;
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, flags)) == NULL) {
        return -1;
    }

    switch (channel_type) {

    case DEVICE:
        rv =  pirate_device_set_param(&channel->device, &param->device);
        break;

    case PIPE:
        rv =  pirate_pipe_set_param(&channel->pipe, &param->pipe);
        break;

    case UNIX_SOCKET:
        rv = pirate_unix_socket_set_param(&channel->unix_socket,
                                            &param->unix_socket);
        break;

    case TCP_SOCKET:
        rv = pirate_tcp_socket_set_param(&channel->tcp_socket,
                                            &param->tcp_socket);
        break;

    case UDP_SOCKET:
        rv = pirate_udp_socket_set_param(&channel->udp_socket,
                                            &param->udp_socket);
        break;

    case SHMEM:
        rv = pirate_shmem_set_param(&channel->shmem, &param->shmem);
        break;

    case UDP_SHMEM:
        rv = pirate_udp_shmem_set_param(&channel->udp_shmem, &param->udp_shmem);
        break;

    case UIO_DEVICE:
        rv = pirate_uio_set_param(&channel->uio, &param->uio);
        break;

    case SERIAL:
        rv = pirate_serial_set_param(&channel->serial, &param->serial);
        break;

    case MERCURY:
        rv = pirate_mercury_set_param(&channel->mercury, &param->mercury);
        break;

    case GE_ETH:
        rv = pirate_ge_eth_set_param(&channel->ge_eth, &param->ge_eth);
        break;

    default:
        errno = EINVAL;
        break;
    }

    if (rv == 0) {
        channel->type = channel_type;
    }

    return rv;
}


channel_t pirate_get_channel_param(int gd, int flags,
                                    pirate_channel_param_t *param) {
    int sts = -1;
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, flags)) == NULL) {
        return INVALID;
    }

    switch (channel->type) {

    case DEVICE:
        sts = pirate_device_get_param(&channel->device, &param->device);
        break;

    case PIPE:
        sts = pirate_pipe_get_param(&channel->pipe, &param->pipe);
        break;

    case UNIX_SOCKET:
        sts = pirate_unix_socket_get_param(&channel->unix_socket,
                                            &param->unix_socket);
        break;

    case TCP_SOCKET:
        sts = pirate_tcp_socket_get_param(&channel->tcp_socket,
                                            &param->tcp_socket);
        break;

    case UDP_SOCKET:
        sts = pirate_udp_socket_get_param(&channel->udp_socket,
                                            &param->udp_socket);
        break;

    case SHMEM:
        sts = pirate_shmem_get_param(&channel->shmem, &param->shmem);
        break;

    case UDP_SHMEM:
        sts = pirate_udp_shmem_get_param(&channel->udp_shmem,
                                            &param->udp_shmem);
        break;

    case UIO_DEVICE:
        sts = pirate_uio_get_param(&channel->uio, &param->uio);
        break;

    case SERIAL:
        sts = pirate_serial_get_param(&channel->serial, &param->serial);
        break;

    case MERCURY:
        sts = pirate_mercury_get_param(&channel->mercury, &param->mercury);
        break;

    case GE_ETH:
        sts = pirate_ge_eth_get_param(&channel->ge_eth, &param->ge_eth);
        break;

    case INVALID:
    default:
        memset(param, 0, sizeof(pirate_channel_param_t));
        sts = 0;
        break;
    }

    if (sts == 0) {
        return channel->type;
    }

    return INVALID;
}


// gaps descriptors must be opened from smallest to largest
int pirate_open(int gd, int flags) {
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, flags)) == NULL) {
        return -1;
    }

    switch (channel->type) {

    case DEVICE:
        return pirate_device_open(gd, flags, &channel->device);

    case PIPE:
        return pirate_pipe_open(gd, flags, &channel->pipe);

    case UNIX_SOCKET:
        return pirate_unix_socket_open(gd, flags, &channel->unix_socket);

    case TCP_SOCKET:
        return pirate_tcp_socket_open(gd, flags, &channel->tcp_socket);

    case UDP_SOCKET:
        return pirate_udp_socket_open(gd, flags, &channel->udp_socket);

    case SHMEM:
        return pirate_shmem_open(gd, flags, &channel->shmem);

    case UDP_SHMEM:
        return pirate_udp_shmem_open(gd, flags, &channel->udp_shmem);

    case UIO_DEVICE:
        return pirate_uio_open(gd, flags, &channel->uio);

    case SERIAL:
        return pirate_serial_open(gd, flags, &channel->serial);

    case MERCURY:
        return pirate_mercury_open(gd, flags, &channel->mercury);

    case GE_ETH:
        return pirate_ge_eth_open(gd, flags, &channel->ge_eth);

    case INVALID:
    default:
        break;
    }

    errno = ENODEV;
    return -1;
}


int pirate_close(int gd, int flags) {
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, flags)) == NULL) {
        return -1;
    }

    switch (channel->type) {

    case DEVICE:
        return pirate_device_close(&channel->device);

    case PIPE:
        return pirate_pipe_close(&channel->pipe);

    case UNIX_SOCKET:
        return pirate_unix_socket_close(&channel->unix_socket);

    case TCP_SOCKET:
        return pirate_tcp_socket_close(&channel->tcp_socket);

    case UDP_SOCKET:
        return pirate_udp_socket_close(&channel->udp_socket);

    case SHMEM:
        return pirate_shmem_close(&channel->shmem);

    case UDP_SHMEM:
        return pirate_udp_shmem_close(&channel->udp_shmem);

    case UIO_DEVICE:
        return pirate_uio_close(&channel->uio);

    case SERIAL:
        return pirate_serial_close(&channel->serial);

    case MERCURY:
        return pirate_mercury_close(&channel->mercury);

    case GE_ETH:
        return pirate_ge_eth_close(&channel->ge_eth);

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, O_RDONLY)) == NULL) {
        return -1;
    }

    switch (channel->type) {

    case DEVICE:
        return pirate_device_read(&channel->device, buf, count);

    case PIPE:
        return pirate_pipe_read(&channel->pipe, buf, count);

    case UNIX_SOCKET:
        return pirate_unix_socket_read(&channel->unix_socket, buf, count);

    case TCP_SOCKET:
        return pirate_tcp_socket_read(&channel->tcp_socket, buf, count);

    case UDP_SOCKET:
        return pirate_udp_socket_read(&channel->udp_socket, buf, count);

    case SHMEM:
        return pirate_shmem_read(&channel->shmem, buf, count);

    case UDP_SHMEM:
        return pirate_udp_shmem_read(&channel->udp_shmem, buf, count);

    case UIO_DEVICE:
        return pirate_uio_read(&channel->uio, buf, count);

    case SERIAL:
        return pirate_serial_read(&channel->serial, buf, count);

    case MERCURY:
        return pirate_mercury_read(&channel->mercury, buf, count);

    case GE_ETH:
        return pirate_ge_eth_read(&channel->ge_eth, buf, count);

    case INVALID:
    default:
        errno = ENODEV;
        return -1;
    }
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
    pirate_channel_ctx_t *channel = NULL;

    if ((channel = pirate_get_channel(gd, O_WRONLY)) == NULL) {
        return -1;
    }

    switch (channel->type) {

    case INVALID:
        errno = ENODEV;
        return -1;

    case DEVICE:
        return pirate_device_write(&channel->device, buf, count);

    case PIPE:
        return pirate_pipe_write(&channel->pipe, buf, count);

    case UNIX_SOCKET:
        return pirate_unix_socket_write(&channel->unix_socket, buf, count);

    case TCP_SOCKET:
        return pirate_tcp_socket_write(&channel->tcp_socket, buf, count);

    case UDP_SOCKET:
        return pirate_udp_socket_write(&channel->udp_socket, buf, count);

    case SHMEM:
        return pirate_shmem_write(&channel->shmem, buf, count);

    case UDP_SHMEM:
        return pirate_udp_shmem_write(&channel->udp_shmem, buf, count);

    case UIO_DEVICE:
        return pirate_uio_write(&channel->uio, buf, count);

    case SERIAL:
        return pirate_serial_write(&channel->serial, buf, count);

    case MERCURY:
        return pirate_mercury_write(&channel->mercury, buf, count);

    case GE_ETH:
        return pirate_ge_eth_write(&channel->ge_eth, buf, count);

    default:
        break;
    }
}
