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

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "pirate_common.h"
#include "udp_socket.h"

static void pirate_udp_socket_init_param(pirate_udp_socket_param_t *param) {
    if (param->mtu == 0) {
        param->mtu = PIRATE_DEFAULT_UDP_PACKET_SIZE;
    }
    if (strnlen(param->reader_addr, 1) == 0) {
        strncpy(param->reader_addr, "0.0.0.0", sizeof(param->reader_addr) - 1);
    }
    if (strnlen(param->writer_addr, 1) == 0) {
        strncpy(param->writer_addr, "0.0.0.0", sizeof(param->writer_addr) - 1);
    }
}

int pirate_udp_socket_parse_param(char *str, void *_param) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "udp_socket") != 0)) {
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->reader_addr, ptr, sizeof(param->reader_addr) - 1);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->reader_port = strtol(ptr, NULL, 10);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->writer_addr, ptr, sizeof(param->writer_addr) - 1);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->writer_port = strtol(ptr, NULL, 10);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("buffer_size", key, strlen("buffer_size")) == 0) {
            param->buffer_size = strtol(val, NULL, 10);
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_udp_socket_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_udp_socket_param_t *param = (const pirate_udp_socket_param_t *)_param;
    char buffer_size_str[32];
    char mtu_str[32];

    buffer_size_str[0] = 0;
    mtu_str[0] = 0;
    if ((param->mtu != 0) && (param->mtu != PIRATE_DEFAULT_UDP_PACKET_SIZE)) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    if (param->buffer_size != 0) {
        snprintf(buffer_size_str, 32, ",buffer_size=%u", param->buffer_size);
    }
    return snprintf(desc, len, "udp_socket,%s,%u,%s,%u%s%s",
        param->reader_addr, param->reader_port,
        param->writer_addr, param->writer_port,
        buffer_size_str, mtu_str);
}

static int populate_address(struct addrinfo *addr, int port, int *addr_any) {
    struct sockaddr_in* ip4_addr;
    struct sockaddr_in6* ip6_addr;

    switch (addr->ai_family) {
        case AF_INET:
            ip4_addr = ((struct sockaddr_in*) addr->ai_addr);
            *addr_any = ip4_addr->sin_addr.s_addr == INADDR_ANY;
            ip4_addr->sin_port = htons(port);
            return 0;
        case AF_INET6:
            ip6_addr = ((struct sockaddr_in6*) addr->ai_addr);
            *addr_any = memcmp((void*) &ip6_addr->sin6_addr, (void*) &in6addr_any, sizeof(struct in6_addr)) == 0;
            ip6_addr->sin6_port = htons(port);
            return 0;
        default:
            return -1;
    }
}

int pirate_udp_socket_reader_open(pirate_udp_socket_param_t *param, common_ctx *ctx) {
    int err, rv;
    struct addrinfo hints, *src_addr = NULL, *dest_addr = NULL;
    int src_addr_any, dest_addr_any;
    int nonblock = ctx->flags & O_NONBLOCK;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_NUMERICHOST;
    src_addr = NULL;
    dest_addr = NULL;

    rv = getaddrinfo(param->reader_addr, NULL, &hints, &src_addr);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = getaddrinfo(param->writer_addr, NULL, &hints, &dest_addr);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    if (src_addr->ai_family != dest_addr->ai_family) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = populate_address(src_addr, param->reader_port, &src_addr_any);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = populate_address(dest_addr, param->writer_port, &dest_addr_any);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    ctx->fd = socket(src_addr->ai_family, SOCK_DGRAM | nonblock, 0);
    if (ctx->fd < 0) {
        goto end;
    }

    int enable = 1;
    rv = setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        err = errno;
        close(ctx->fd);
        ctx->fd = -1;
        errno = err;
        goto end;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->fd, SOL_SOCKET, SO_RCVBUF,
                        &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(ctx->fd);
            ctx->fd = -1;
            errno = err;
            goto end;
        }
    }

    rv = bind(ctx->fd, src_addr->ai_addr, src_addr->ai_addrlen);
    if (rv < 0) {
        err = errno;
        close(ctx->fd);
        ctx->fd = -1;
        errno = err;
        goto end;
    }
    if (!dest_addr_any) {
        rv = connect(ctx->fd, dest_addr->ai_addr, dest_addr->ai_addrlen);
        if (rv < 0) {
            err = errno;
            close(ctx->fd);
            ctx->fd = -1;
            errno = err;
            goto end;
        }
    }

end:
    if (src_addr != NULL) {
        freeaddrinfo(src_addr);
    }
    if (dest_addr != NULL) {
        freeaddrinfo(dest_addr);
    }
    return ctx->fd;
}

int pirate_udp_socket_writer_open(pirate_udp_socket_param_t *param, common_ctx *ctx) {
    int err, rv;
    struct addrinfo hints, *src_addr = NULL, *dest_addr = NULL;
    int src_addr_any, dest_addr_any;
    int nonblock = ctx->flags & O_NONBLOCK;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_NUMERICHOST;
    src_addr = NULL;
    dest_addr = NULL;

    rv = getaddrinfo(param->writer_addr, NULL, &hints, &src_addr);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = getaddrinfo(param->reader_addr, NULL, &hints, &dest_addr);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    if (src_addr->ai_family != dest_addr->ai_family) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = populate_address(src_addr, param->writer_port, &src_addr_any);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    rv = populate_address(dest_addr, param->reader_port, &dest_addr_any);
    if (rv != 0) {
        errno = EINVAL;
        ctx->fd = -1;
        goto end;
    }

    ctx->fd = socket(src_addr->ai_family, SOCK_DGRAM | nonblock, 0);
    if (ctx->fd < 0) {
        goto end;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->fd, SOL_SOCKET, SO_SNDBUF,
                        &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(ctx->fd);
            ctx->fd = -1;
            errno = err;
            goto end;
        }
    }

    if (!src_addr_any) {
        rv = bind(ctx->fd, src_addr->ai_addr, src_addr->ai_addrlen);
        if (rv < 0) {
            err = errno;
            close(ctx->fd);
            errno = err;
            goto end;
        }
    }
    rv = connect(ctx->fd, dest_addr->ai_addr, dest_addr->ai_addrlen);
    if (rv < 0) {
        err = errno;
        close(ctx->fd);
        ctx->fd = -1;
        errno = err;
        goto end;
    }

end:
    if (src_addr != NULL) {
        freeaddrinfo(src_addr);
    }
    if (dest_addr != NULL) {
        freeaddrinfo(dest_addr);
    }
    return ctx->fd;
}

int pirate_udp_socket_open(void *_param, void *_ctx) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;

    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_udp_socket_init_param(param);
    if (param->reader_port <= 0) {
        errno = EINVAL;
        return -1;
    }
    if (param->writer_port < 0) {
        errno = EINVAL;
        return -1;
    }
    if ((param->writer_port > 0) && (strncmp(param->writer_addr, "0.0.0.0", 8) == 0)) {
        errno = EINVAL;
        return -1;
    }
    if (param->mtu > PIRATE_DEFAULT_UDP_PACKET_SIZE) {
        errno = EINVAL;
        return -1;
    }
    if (access == O_RDONLY) {
        rv = pirate_udp_socket_reader_open(param, (common_ctx*) ctx);
    } else {
        rv = pirate_udp_socket_writer_open(param, (common_ctx*) ctx);
    }

    return rv;
}

int pirate_udp_socket_close(void *_ctx) {
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;
    int err, rv = -1;

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    err = errno;
    shutdown(ctx->sock, SHUT_RDWR);
    errno = err;

    rv = close(ctx->sock);
    ctx->sock = -1;
    return rv;
}

ssize_t pirate_udp_socket_read(const void *_param, void *_ctx, void *buf, size_t count) {
    (void) _param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    return recv(ctx->sock, buf, count, 0);
}

ssize_t pirate_udp_socket_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        mtu = PIRATE_DEFAULT_UDP_PACKET_SIZE;
    }
    if (mtu > PIRATE_DEFAULT_UDP_PACKET_SIZE) {
        errno = EINVAL;
        return -1;
    }
    // 8 byte UDP header and 20 byte IP header
    if (mtu < 28) {
        errno = EINVAL;
        return -1;
    }
    return mtu - 28;
}

ssize_t pirate_udp_socket_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;
    int err;
    ssize_t rv;
    size_t write_mtu = pirate_udp_socket_write_mtu(param, ctx);

    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }
    if ((write_mtu > 0) && (count > write_mtu)) {
        errno = EMSGSIZE;
        return -1;
    }
    err = errno;
    rv = send(ctx->sock, buf, count, 0);
    if ((rv < 0) && (errno == ECONNREFUSED)) {
        // TODO create a counter of undelivered messages
        errno = err;
        rv = send(ctx->sock, buf, count, 0);
    }
    return rv;
}
