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

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "pirate_common.h"
#include "tcp_socket.h"

int pirate_tcp_socket_init_param(int gd, int flags,
                                    pirate_tcp_socket_param_t *param) {
    (void) flags;
    snprintf(param->addr, sizeof(param->addr) - 1, DEFAULT_TCP_IP_ADDR);
    param->port = PIRATE_TCP_PORT_BASE + gd;
    param->iov_len = 0;
    param->buffer_size = 0;
    return 0;
}

int pirate_tcp_socket_parse_param(int gd, int flags, char *str,
                                    pirate_tcp_socket_param_t *param) {
    char *ptr = NULL;

    pirate_tcp_socket_init_param(gd, flags, param);

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "tcp_socket") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->addr, ptr, sizeof(param->addr));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->port = strtol(ptr, NULL, 10);
    }
    
    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->iov_len = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->buffer_size = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_tcp_socket_set_param(pirate_tcp_socket_ctx_t *ctx,
                                    const pirate_tcp_socket_param_t *param) {
    if (param == NULL) {
        memset(&ctx->param, 0, sizeof(ctx->param));
    } else {
        ctx->param = *param;
    }
    
    return 0;
}

int pirate_tcp_socket_get_param(const pirate_tcp_socket_ctx_t *ctx,
                                    pirate_tcp_socket_param_t *param) {
    *param  = ctx->param;
    return 0;
}

static int tcp_socket_reader_open(pirate_tcp_socket_ctx_t *ctx) {
    int rv;
    int server_fd;
    struct sockaddr_in addr;
    const pirate_tcp_socket_param_t *param = &ctx->param;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return server_fd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(param->port);

    int enable = 1;
    rv = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        int err = errno;
        close(server_fd);
        errno = err;
        return rv;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            int err = errno;
            close(server_fd);
            errno = err;
            return rv;
        }
    }

    rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
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

static int tcp_socket_writer_open(pirate_tcp_socket_ctx_t *ctx) {
    int rv;
    struct sockaddr_in addr;
    const pirate_tcp_socket_param_t *param = &ctx->param;

    ctx->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                    sizeof(param->buffer_size));
        if (rv < 0) {
            int err = errno;
            close(ctx->sock);
            ctx->sock = -1;
            errno = err;
            return rv;
        }
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(param->addr);
    addr.sin_port = htons(param->port);

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
            ctx->sock = -1;
            errno = err;
            return rv;
        }

        return 0;
    }

    return -1;
}

int pirate_tcp_socket_open(int gd, int flags, pirate_tcp_socket_ctx_t *ctx) {
    int rv = -1;
    if (flags == O_RDONLY) {
        rv = tcp_socket_reader_open(ctx);
    } else if (flags == O_WRONLY) {
        rv = tcp_socket_writer_open(ctx);
    }

    return rv == 0 ? gd : rv;
}

int pirate_tcp_socket_close(pirate_tcp_socket_ctx_t *ctx) {
    int rv = -1;

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->sock);
    ctx->sock = -1;
    return rv;
}

ssize_t pirate_tcp_socket_read(pirate_tcp_socket_ctx_t *ctx, void *buf,
                                size_t count) {
    return pirate_fd_read(ctx->sock, buf, count, ctx->param.iov_len);
}

ssize_t pirate_tcp_socket_write(pirate_tcp_socket_ctx_t *ctx, const void *buf,
                                    size_t count) {
    return pirate_fd_write(ctx->sock, buf, count, ctx->param.iov_len);
}
