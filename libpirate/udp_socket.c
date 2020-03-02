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
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "pirate_common.h"
#include "udp_socket.h"

#define DEFAULT_UDP_IP_ADDR     "127.0.0.1"
#define PIRATE_UDP_PORT_BASE    26427

int pirate_udp_socket_init_param(int gd, int flags, 
                                    pirate_udp_socket_param_t *param) {
    (void) flags;
    snprintf(param->addr, sizeof(param->addr) - 1, DEFAULT_UDP_IP_ADDR);
    param->port = PIRATE_UDP_PORT_BASE + gd;
    param->iov_len = 0;
    param->buffer_size = 0;
    return 0;
}

int pirate_udp_socket_parse_param(char *str, 
                                    pirate_udp_socket_param_t *param) {
    char *ptr = NULL;

    pirate_udp_socket_init_param(0, 0, param);

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "udp_socket") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->addr, ptr, sizeof(param->addr));

    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->port = strtol(ptr, NULL, 10);

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->iov_len = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->buffer_size = strtol(ptr, NULL, 10);
    }

    return 0;
}
             
int pirate_udp_socket_set_param(pirate_udp_socket_ctx_t *ctx,
                                    const pirate_udp_socket_param_t *param) {
    if (param == NULL) {
        memset(&ctx->param, 0, sizeof(ctx->param));
    } else {
        ctx->param = *param;
    }
    
    return 0;
}

int pirate_udp_socket_get_param(const pirate_udp_socket_ctx_t *ctx,
                                    pirate_udp_socket_param_t *param) {
    *param  = ctx->param;
    return 0;
}

static int udp_socket_reader_open(pirate_udp_socket_ctx_t *ctx) {
    int rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(ctx->param.port);

    int enable = 1;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        int err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    if (ctx->param.buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_RCVBUF, 
                        &ctx->param.buffer_size, 
                        sizeof(ctx->param.buffer_size));
        if (rv < 0) {
            int err = errno;
            close(ctx->sock);
            ctx->sock = -1;
            errno = err;
            return rv;
        }
    }

    rv = bind(ctx->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        int err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    return 0;
}

static int udp_socket_writer_open(pirate_udp_socket_ctx_t *ctx) {
    int rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    if (ctx->param.buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, 
                        &ctx->param.buffer_size,
                        sizeof(ctx->param.buffer_size));
        if (rv < 0) {
            int err = errno;
            close(ctx->sock);
            ctx->sock = -1;
            errno = err;
            return rv;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ctx->param.addr);
    addr.sin_port = htons(ctx->param.port);
    rv = connect(ctx->sock, (const struct sockaddr*) &addr, sizeof(addr));
    if (rv < 0) {
        int err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    return 0;
}

int pirate_udp_socket_open(int gd, int flags, pirate_udp_socket_ctx_t *ctx) {
    (void) gd;
    if (flags == O_RDONLY) {
        return udp_socket_reader_open(ctx);
    }

    return udp_socket_writer_open(ctx);
}

int pirate_udp_socket_close(pirate_udp_socket_ctx_t *ctx) {
    int rv = -1;

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->sock);
    ctx->sock = -1;
    return rv;
}


static int pirate_make_msgvec(void *buf, size_t count, size_t iov_len, 
                             struct mmsghdr *msgvec, struct iovec *iov) {
    unsigned char *iov_base = buf;
    int vlen = count / iov_len;
    if (count > iov_len * vlen) {
        ++vlen;
    }

    vlen = MIN(vlen, PIRATE_IOV_MAX);
    memset(msgvec, 0, sizeof(struct mmsghdr) * PIRATE_IOV_MAX);

    for (int i = 0; i < vlen; i++) {
        iov[i].iov_base = iov_base;
        iov[i].iov_len = MIN(count, iov_len);
        msgvec[i].msg_hdr.msg_name = NULL;
        msgvec[i].msg_hdr.msg_namelen = 0;
        msgvec[i].msg_hdr.msg_iov = &iov[i];
        msgvec[i].msg_hdr.msg_iovlen = 1;
        iov_base += iov[i].iov_len;
        count -= iov[i].iov_len;
    }

    return vlen;
}

ssize_t pirate_udp_socket_read(pirate_udp_socket_ctx_t *ctx, void *buf,
                                size_t count) {
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    if ((ctx->param.iov_len > 0) && (ctx->param.iov_len)) {
        struct mmsghdr msgvec[PIRATE_IOV_MAX];
        struct iovec iov[PIRATE_IOV_MAX];
        int vlen = pirate_make_msgvec(buf, count, ctx->param.iov_len, 
            msgvec, iov);
        int rd_bytes = 0;

        int rv = recvmmsg(ctx->sock, msgvec, vlen, 0, NULL);
        if (rv < 0) {
            return rv;
        }
        
        for (int i = 0; i < rv; i++) {
            rd_bytes += iov[i].iov_len;
        }
        return rd_bytes;
    }

    return recv(ctx->sock, buf, count, 0);
}

ssize_t pirate_udp_socket_write(pirate_udp_socket_ctx_t *ctx, const void *buf,
                                    size_t count) {
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    if ((ctx->param.iov_len > 0) && (ctx->param.iov_len)) {
        struct mmsghdr msgvec[PIRATE_IOV_MAX];
        struct iovec iov[PIRATE_IOV_MAX];
        int vlen = pirate_make_msgvec((void *)buf, count, ctx->param.iov_len, 
            msgvec, iov);
        int wr_bytes = 0;

        int rv = sendmmsg(ctx->sock, msgvec, vlen, 0);
        if (rv < 0) {
            return rv;
        }

        for (int i = 0; i < rv; i++) {
            wr_bytes += iov[i].iov_len;
        }
        
        return wr_bytes;
    }

    return send(ctx->sock, buf, count, 0);
}
