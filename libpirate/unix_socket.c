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

static void pirate_unix_socket_init_param(int gd, pirate_unix_socket_param_t *param) {
    if (strnlen(param->path, 1) == 0) {
        snprintf(param->path, PIRATE_LEN_NAME - 1, PIRATE_UNIX_SOCKET_NAME_FMT, gd);
    }
}

int pirate_unix_socket_parse_param(char *str, pirate_unix_socket_param_t *param) {
    char *ptr = NULL;

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) ||
        (strcmp(ptr, "unix_socket") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->path, ptr, sizeof(param->path));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->iov_len = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->buffer_size = strtol(ptr, NULL, 10);
    }

    return 0;
}

static int unix_socket_reader_open(pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    int server_fd;
    int rv;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return server_fd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, param->path, PIRATE_LEN_NAME - 1);

    if (param->buffer_size > 0) {
        rv = setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            int err = errno;
            close(server_fd);
            errno = err;
            return rv;
        }
    }

    unlink(param->path);
    // ignore unlink error if file does not exist
    errno = 0;
    rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (rv < 0) {
        int err = errno;
        close(server_fd);
        errno = err;
        return rv;
    }

    rv = listen(server_fd, 0);
    if (rv < 0) {
        int err = errno;
        close(server_fd);
        errno = err;
        return rv;
    }

    ctx->sock = accept(server_fd, NULL, NULL);

    if (ctx->sock < 0) {
        int err = errno;
        close(server_fd);
        errno = err;
        return ctx->sock;
    }

    close(server_fd);
    return 0;
}

static int unix_socket_writer_open(pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    struct sockaddr_un addr;
    int rv;

    ctx->sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, param->path, PIRATE_LEN_NAME - 1);

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            int err = errno;
            close(ctx->sock);
            errno = err;
            return rv;
        }
    }

    for (;;) {
        rv = connect(ctx->sock, (struct sockaddr *)&addr, sizeof(addr));
        if (rv < 0) {
            if ((errno == ENOENT) || (errno == ECONNREFUSED)) {
                struct timespec req;
                errno = 0;
                req.tv_sec = 0;
                req.tv_nsec = 1e8;
                rv = nanosleep(&req, NULL);
                if (rv == 0) {
                    continue;
                }
            }
            int err = errno;
            close(ctx->sock);
            errno = err;
            return rv;
        }

        return 0;
    }

    return -1;
}

int pirate_unix_socket_open(int gd, int flags, pirate_unix_socket_param_t *param, unix_socket_ctx *ctx) {
    int rv = -1;

    pirate_unix_socket_init_param(gd, param);
    if (flags == O_RDONLY) {
        rv = unix_socket_reader_open(param, ctx);
    } else if (flags == O_WRONLY) {
        rv = unix_socket_writer_open(param, ctx);
    }

    return rv == 0 ? gd : rv;
}


int pirate_unix_socket_close(unix_socket_ctx *ctx) {
    int rv = -1;

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->sock);
    ctx->sock = -1;
    return rv;
}

ssize_t pirate_unix_socket_read(const pirate_unix_socket_param_t *param, unix_socket_ctx *ctx, void *buf, size_t count) {
    return pirate_fd_read(ctx->sock, buf, count, param->iov_len);
}

ssize_t pirate_unix_socket_write(const pirate_unix_socket_param_t *param, unix_socket_ctx *ctx, const void *buf, size_t count) {
    return pirate_fd_write(ctx->sock, buf, count, param->iov_len);
}
