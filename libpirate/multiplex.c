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

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>

#include "pirate_common.h"
#include "multiplex.h"

int pirate_multiplex_parse_param(char *str, void *_param) {
    (void) str;
    (void) _param;
    return 0;
}

int pirate_multiplex_get_channel_description(const void *_param, char *desc, int len) {
    (void) _param;
    return snprintf(desc, len, "multiplex");
}

int pirate_multiplex_open(void *_param, void *_ctx, int *server_fdp) {
    (void) _param;
    (void) server_fdp;
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    ctx->count = 0;
    memset(ctx->fds, 0, sizeof(ctx->fds));
    memset(ctx->gds, 0, sizeof(ctx->gds));
    return 0;
}

int pirate_multiplex_close(void *_ctx) {
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    int rv = 0;
    for (int i = 0; i < ctx->count; i++) {
        int local = pirate_close(ctx->gds[i]);
        if (!local) {
            rv = local;
        }
    }
    return rv;
}

int pirate_multiplex_add(void *_ctx, int gd) {
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    if (ctx->count == PIRATE_MULTIPLEX_NUM_CHANNELS) {
        errno = EMLINK;
        return -1;
    }
    int fd = pirate_get_fd(gd);
    if (fd < 0) {
        return -1;
    }
    ctx->gds[ctx->count] = gd;
    ctx->fds[ctx->count] = fd;
    ctx->count++;
    return 0;
}

ssize_t pirate_multiplex_read(const void *_param, void *_ctx, void *buf, size_t count) {
    (void) _param;
    struct pollfd fds[PIRATE_MULTIPLEX_NUM_CHANNELS];
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    int nonblock = ctx->flags & O_NONBLOCK;
    int timeout = (nonblock) ? 0 : -1;
    if (!ctx->count) {
        errno = ENXIO;
        return -1;
    }
    for (int i = 0; i < ctx->count; i++) {
        fds[i].fd = ctx->fds[i];
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    int rv = poll(fds, ctx->count, timeout);
    if (rv == 0) {
        errno = EAGAIN;
        return -1;
    } else if (rv < 0) {
        return rv;
    }
    for (int i = 0; i < ctx->count; i++) {
        if (fds[i].revents & POLLIN) {
            return pirate_read(ctx->gds[i], buf, count);
        }
    }
    return 0;
}

// the write mtu of the multiplex channel is computed
// from the write mtu of its component channels.
// If all write mtus are zero then the multiplex write
// mtu is zero. Otherwise the multiplex write mtu is
// the smallest component write mtu that is non-zero.
ssize_t pirate_multiplex_write_mtu(const void *_param, void *_ctx) {
    (void) _param;
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    ssize_t rv = 0;
    if (ctx == NULL) {
        return 0;
    }
    if (!ctx->count) {
        errno = ENXIO;
        return -1;
    }
    for (int i = 0; i < ctx->count; i++) {
        ssize_t local = pirate_write_mtu(ctx->gds[i]);
        if (local < 0) {
            return local;
        } else if (local == 0) {
            continue;
        } else if ((rv == 0) || (local < rv)) {
            rv = local;
        }
    }
    return rv;
}

ssize_t pirate_multiplex_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    multiplex_ctx *ctx = (multiplex_ctx *)_ctx;
    size_t mtu = pirate_multiplex_write_mtu(_param, ctx);
    if (!ctx->count) {
        errno = ENXIO;
        return -1;
    }
    if ((mtu > 0) && (count > mtu)) {
        errno = EMSGSIZE;
        return -1;
    }
    for (int i = 0; i < ctx->count; i++) {
        int rv = pirate_write(ctx->gds[i], buf, count);
        if (rv < 0) {
            return rv;
        }
    }
    return count;
}
