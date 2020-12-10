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
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pirate_common.h"
#include "pipe.h"

static void pirate_pipe_init_param(pirate_pipe_param_t *param) {
    if (param->min_tx == 0) {
        param->min_tx = PIRATE_DEFAULT_MIN_TX;
    }
}

int pirate_pipe_parse_param(char *str, void *_param) {
    pirate_pipe_param_t *param = (pirate_pipe_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "pipe") != 0)) {
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->path, ptr, sizeof(param->path) - 1);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("min_tx_size", key, strlen("min_tx_size")) == 0) {
            param->min_tx = strtol(val, NULL, 10);
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_pipe_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_pipe_param_t *param = (const pirate_pipe_param_t *)_param;
    char min_tx_str[32];
    char mtu_str[32];

    min_tx_str[0] = 0;
    mtu_str[0] = 0;
    if (param->min_tx != 0) {
        snprintf(min_tx_str, 32, ",min_tx_size=%u", param->min_tx);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    return snprintf(desc, len, "pipe,%s%s%s", param->path, min_tx_str, mtu_str);
}

int pirate_pipe_open(void *_param, void *_ctx) {
    pirate_pipe_param_t *param = (pirate_pipe_param_t *)_param;
    pipe_ctx *ctx = (pipe_ctx *)_ctx;
    int nonblock = ctx->flags & O_NONBLOCK;
    int flags = ctx->flags & ~O_NONBLOCK; // O_NONBLOCK | O_WRONLY can generate ENXIO on open()
    int status_flags, new_status_flags;
    int err;

    pirate_pipe_init_param(param);
    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    err = errno;
    if (mkfifo(param->path, 0660) == -1) {
        if (errno == EEXIST) {
            errno = err;
        } else {
            return -1;
        }
    }

    if ((ctx->fd = open(param->path, flags)) < 0) {
        return -1;
    }

    status_flags = fcntl(ctx->fd, F_GETFL, NULL);
    if (status_flags < 0) {
        return -1;
    }
    if (nonblock) {
        new_status_flags = status_flags | O_NONBLOCK;
    } else {
        new_status_flags = status_flags & ~O_NONBLOCK;
    }
    if (new_status_flags != status_flags) {
        if (fcntl(ctx->fd, F_SETFL, new_status_flags) < 0) {
            return -1;
        }
    }

    if (nonblock) {
        // ensure that one read() or write() consumes the entire datagram
        param->min_tx = param->mtu;
    }

    if ((ctx->min_tx_buf = calloc(param->min_tx, 1)) == NULL) {
        return -1;
    }

    return ctx->fd;
}

int pirate_pipe_close(void *_ctx) {
    pipe_ctx *ctx = (pipe_ctx *)_ctx;
    int rv = -1;

    if (ctx->min_tx_buf != NULL) {
        free(ctx->min_tx_buf);
        ctx->min_tx_buf = NULL;
    }

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_pipe_read(const void *_param, void *_ctx, void *buf, size_t count) {
    const pirate_pipe_param_t *param = (const pirate_pipe_param_t *)_param;
    return pirate_stream_read((common_ctx*) _ctx, param->min_tx, buf, count);
}

ssize_t pirate_pipe_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_pipe_param_t *param = (const pirate_pipe_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        return 0;
    }
    if (mtu < sizeof(pirate_header_t)) {
        errno = EINVAL;
        return -1;
    }
    return mtu - sizeof(pirate_header_t);
}

ssize_t pirate_pipe_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_pipe_param_t *param = (const pirate_pipe_param_t *)_param;
    ssize_t mtu = pirate_pipe_write_mtu(param, _ctx);
    return pirate_stream_write((common_ctx*)_ctx, param->min_tx, mtu, buf, count);
}
