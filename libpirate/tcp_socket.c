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

static void pirate_tcp_socket_init_param(pirate_tcp_socket_param_t *param) {
    if (param->min_tx == 0) {
        param->min_tx = PIRATE_DEFAULT_MIN_TX;
    }
}

int pirate_tcp_socket_parse_param(char *str, void *_param) {
    pirate_tcp_socket_param_t *param = (pirate_tcp_socket_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "tcp_socket") != 0)) {
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->addr, ptr, sizeof(param->addr) - 1);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->port = strtol(ptr, NULL, 10);

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
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_tcp_socket_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_tcp_socket_param_t *param = (const pirate_tcp_socket_param_t *)_param;
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
    return snprintf(desc, len, "tcp_socket,%s,%u%s%s%s", param->addr, param->port,
        buffer_size_str, min_tx_str, mtu_str);
}

static int tcp_socket_reader_open(pirate_tcp_socket_param_t *param, tcp_socket_ctx *ctx, int *server_fdp) {
    int err, rv;
    int server_fd;
    struct sockaddr_in addr;
    struct linger lo;

    if ((server_fdp != NULL) && (*server_fdp > 0)) {
        server_fd = *server_fdp;
    } else {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            return server_fd;
        }

        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(param->addr);
        addr.sin_port = htons(param->port);

        int enable = 1;
        rv = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
        if (rv < 0) {
            err = errno;
            close(server_fd);
            errno = err;
            return rv;
        }

        if (param->buffer_size > 0) {
            rv = setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &param->buffer_size,
                        sizeof(param->buffer_size));
            if (rv < 0) {
                err = errno;
                close(server_fd);
                errno = err;
                return rv;
            }
        }

        rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
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
    }

    ctx->sock = accept(server_fd, NULL, NULL);

    if (ctx->sock < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        if (server_fdp != NULL) *server_fdp = 0;
        return ctx->sock;
    }

    lo.l_onoff = 1;
    lo.l_linger = 0;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        close(server_fd);
        errno = err;
        if (server_fdp != NULL) *server_fdp = 0;
        return rv;
    }

    if (server_fdp == NULL) {
        close(server_fd);
    } else if (*server_fdp == 0) {
        *server_fdp = server_fd;
    }
    return 0;
}

static int tcp_socket_writer_open(pirate_tcp_socket_param_t *param, tcp_socket_ctx *ctx) {
    int err, rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, &param->buffer_size,
                    sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
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
            ctx->sock = -1;
            errno = err;
            return rv;
        }

        return 0;
    }

    return -1;
}

int pirate_tcp_socket_open(void *_param, void *_ctx, int *server_fdp) {
    pirate_tcp_socket_param_t *param = (pirate_tcp_socket_param_t *)_param;
    tcp_socket_ctx *ctx = (tcp_socket_ctx *)_ctx;
    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_tcp_socket_init_param(param);
    if (param->port <= 0) {
        errno = EINVAL;
        return -1;
    }
    if (access == O_RDONLY) {
        rv = tcp_socket_reader_open(param, ctx, server_fdp);
    } else {
        rv = tcp_socket_writer_open(param, ctx);
    }
    if ((ctx->min_tx_buf = calloc(param->min_tx, 1)) == NULL) {
        return -1;
    }
    return rv;
}

int pirate_tcp_socket_close(void *_ctx) {
    tcp_socket_ctx *ctx = (tcp_socket_ctx *)_ctx;
    int err, rv = -1;
    int access = ctx->flags & O_ACCMODE;

    if (ctx->min_tx_buf != NULL) {
        free(ctx->min_tx_buf);
        ctx->min_tx_buf = NULL;
    }
    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    err = errno;
    shutdown(ctx->sock, SHUT_RDWR);
    errno = err;

    rv = close(ctx->sock);
    ctx->sock = -1;
    // Reader closes with RST packet.
    // If the reader closes before the writer then a
    // connection reset error is expected.
    if ((access == O_WRONLY) && (errno == ENOTCONN)) {
        errno = 0;
    }
    return rv;
}

ssize_t pirate_tcp_socket_read(const void *_param, void *_ctx, void *buf, size_t count) {
    const pirate_tcp_socket_param_t *param = (const pirate_tcp_socket_param_t *)_param;
    return pirate_stream_read((common_ctx*) _ctx, param->min_tx, buf, count);
}

ssize_t pirate_tcp_socket_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_tcp_socket_param_t *param = (const pirate_tcp_socket_param_t *)_param;
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

ssize_t pirate_tcp_socket_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_tcp_socket_param_t *param = (const pirate_tcp_socket_param_t *)_param;
    ssize_t mtu = pirate_tcp_socket_write_mtu(param, _ctx);
    return pirate_stream_write((common_ctx*)_ctx, param->min_tx, mtu, buf, count);
}
