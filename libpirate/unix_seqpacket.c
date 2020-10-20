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

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "pirate_common.h"
#include "unix_seqpacket.h"

static void pirate_unix_seqpacket_init_param(pirate_unix_seqpacket_param_t *param) {
    if (param->min_tx == 0) {
        param->min_tx = PIRATE_DEFAULT_MIN_TX;
    }
}

int pirate_unix_seqpacket_parse_param(char *str, void *_param) {
    pirate_unix_seqpacket_param_t *param = (pirate_unix_seqpacket_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "unix_seqpacket") != 0)) {
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
        if (strncmp("buffer_size", key, strlen("buffer_size")) == 0) {
            param->buffer_size = strtol(val, NULL, 10);
        } else if (strncmp("min_tx_size", key, strlen("min_tx_size")) == 0) {
            param->min_tx = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_unix_seqpacket_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_unix_seqpacket_param_t *param = (const pirate_unix_seqpacket_param_t *)_param;
    char min_tx_str[32];
    char buffer_size_str[32];
    char mtu_str[32];

    min_tx_str[0] = 0;
    buffer_size_str[0] = 0;
    mtu_str[0] = 0;
    if (param->min_tx != 0) {
        snprintf(min_tx_str, 32, ",min_tx_size=%u", param->min_tx);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    if (param->buffer_size != 0) {
        snprintf(buffer_size_str, 32, ",buffer_size=%u", param->buffer_size);
    }
    return snprintf(desc, len, "unix_seqpacket,%s%s%s%s", param->path,
        buffer_size_str, min_tx_str, mtu_str);
}

static int unix_seqpacket_reader_open(pirate_unix_seqpacket_param_t *param, unix_seqpacket_ctx *ctx) {
    int server_fd;
    int err, rv;
    struct sockaddr_un addr;
    int nonblock = ctx->flags & O_NONBLOCK;


    server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (server_fd < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, param->path, sizeof(addr.sun_path) - 1);

    if (param->buffer_size > 0) {
        rv = setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(server_fd);
            errno = err;
            return -1;
        }
    }
    err = errno;
    unlink(param->path);
    // ignore unlink error if file does not exist
    errno = err;
    rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (rv < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return -1;
    }

    rv = listen(server_fd, 0);
    if (rv < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return -1;
    }

    ctx->sock = accept4(server_fd, NULL, NULL, nonblock);

    if (ctx->sock < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return -1;
    }

    err = errno;
    close(server_fd);
    errno = err;
    return ctx->sock;
}

static int unix_seqpacket_writer_open(pirate_unix_seqpacket_param_t *param, unix_seqpacket_ctx *ctx) {
    struct sockaddr_un addr;
    int err, rv;
    int nonblock = ctx->flags & O_NONBLOCK;

    ctx->sock = socket(AF_UNIX, SOCK_SEQPACKET | nonblock, 0);
    if (ctx->sock < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, param->path, sizeof(addr.sun_path) - 1);

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(ctx->sock);
            errno = err;
            return -1;
        }
    }

    for (;;) {
        err = errno;
        rv = connect(ctx->sock, (struct sockaddr *)&addr, sizeof(addr));
        if (rv < 0) {
            if ((errno == ENOENT) || (errno == ECONNREFUSED)) {
                struct timespec req;
                errno = err;
                req.tv_sec = 0;
                req.tv_nsec = 1e8;
                rv = nanosleep(&req, NULL);
                if (rv == 0) {
                    continue;
                }
            }
            err = errno;
            close(ctx->sock);
            errno = err;
            return -1;
        }

        return ctx->sock;
    }

    return -1;
}

int pirate_unix_seqpacket_open(void *_param, void *_ctx) {
    pirate_unix_seqpacket_param_t *param = (pirate_unix_seqpacket_param_t *)_param;
    unix_seqpacket_ctx *ctx = (unix_seqpacket_ctx *)_ctx;
    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_unix_seqpacket_init_param(param);

    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    if (access == O_RDONLY) {
        rv = unix_seqpacket_reader_open(param, ctx);
    } else {
        rv = unix_seqpacket_writer_open(param, ctx);
    }

    if ((ctx->min_tx_buf = calloc(param->min_tx, 1)) == NULL) {
        return -1;
    }

    return rv;
}


int pirate_unix_seqpacket_close(void *_ctx) {
    unix_seqpacket_ctx *ctx = (unix_seqpacket_ctx *)_ctx;
    int rv = -1;

    if (ctx->min_tx_buf != NULL) {
        free(ctx->min_tx_buf);
        ctx->min_tx_buf = NULL;
    }

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->sock);
    ctx->sock = -1;
    return rv;
}

ssize_t pirate_unix_seqpacket_read(const void *_param, void *_ctx, void *buf, size_t count) {
    (void) _param;
    unix_seqpacket_ctx *ctx = (unix_seqpacket_ctx *)_ctx;
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }
    return recv(ctx->sock, buf, count, 0);
}

ssize_t pirate_unix_seqpacket_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_unix_seqpacket_param_t *param = (const pirate_unix_seqpacket_param_t *)_param;
    return param->mtu;
}

ssize_t pirate_unix_seqpacket_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_unix_seqpacket_param_t *param = (const pirate_unix_seqpacket_param_t *)_param;
    unix_seqpacket_ctx *ctx = (unix_seqpacket_ctx *)_ctx;
    size_t write_mtu = pirate_unix_seqpacket_write_mtu(param, ctx);

    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }
    if ((write_mtu > 0) && (count > write_mtu)) {
        errno = EMSGSIZE;
        return -1;
    }
    return send(ctx->sock, buf, count, 0);
}
