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
#include "unix_socket.h"

static void pirate_unix_socket_init_param(pirate_unix_socket_param_t *param) {
    if (param->min_tx == 0) {
        param->min_tx = PIRATE_DEFAULT_MIN_TX;
    }
}

int pirate_unix_socket_parse_param(char *str, pirate_unix_socket_param_t *param) {
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "unix_socket") != 0)) {
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

int pirate_unix_socket_get_channel_description(const pirate_unix_socket_param_t *param,char *desc, int len) {
    char min_tx_str[32];
    char buffer_size_str[32];
    char mtu_str[32];

    min_tx_str[0] = 0;
    buffer_size_str[0] = 0;
    mtu_str[0] = 0;
    if ((param->min_tx != 0) && (param->min_tx != PIRATE_DEFAULT_MIN_TX)) {
        snprintf(min_tx_str, 32, ",min_tx_size=%u", param->min_tx);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    if (param->buffer_size != 0) {
        snprintf(buffer_size_str, 32, ",buffer_size=%u", param->buffer_size);
    }
    return snprintf(desc, len, "unix_socket,%s%s%s%s", param->path,
        buffer_size_str, min_tx_str, mtu_str);
}

static int unix_socket_reader_open(pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    int server_fd;
    int err, rv;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return server_fd;
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
            return rv;
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
        return rv;
    }

    rv = listen(server_fd, 0);
    if (rv < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return rv;
    }

    ctx->sock = accept(server_fd, NULL, NULL);

    if (ctx->sock < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return ctx->sock;
    }

    close(server_fd);
    return 0;
}

static int unix_socket_writer_open(pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    struct sockaddr_un addr;
    int err, rv;

    ctx->sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
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
            return rv;
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
            return rv;
        }

        return 0;
    }

    return -1;
}

int pirate_unix_socket_open(pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_unix_socket_init_param(param);

    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    if (access == O_RDONLY) {
        rv = unix_socket_reader_open(param, ctx);
    } else {
        rv = unix_socket_writer_open(param, ctx);
    }

    if ((ctx->min_tx_buf = calloc(param->min_tx, 1)) == NULL) {
        return -1;
    }

    return rv;
}


int pirate_unix_socket_close(unix_socket_ctx *ctx) {
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

ssize_t pirate_unix_socket_read(const pirate_unix_socket_param_t *param, unix_socket_ctx *ctx, void *buf, size_t count) {
    return pirate_stream_read((common_ctx*) ctx, param->min_tx, buf, count);
}

ssize_t pirate_unix_socket_write_mtu(const pirate_unix_socket_param_t *param) {
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

ssize_t pirate_unix_socket_write(const pirate_unix_socket_param_t *param, unix_socket_ctx *ctx, const void *buf, size_t count) {
    ssize_t mtu = pirate_unix_socket_write_mtu(param);
    return pirate_stream_write((common_ctx*) ctx, param->min_tx, mtu, buf, count);
}
