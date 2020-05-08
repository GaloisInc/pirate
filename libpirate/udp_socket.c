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

static void pirate_udp_socket_init_param(pirate_udp_socket_param_t *param) {
    if (strnlen(param->addr, 1) == 0) {
        snprintf(param->addr, sizeof(param->addr) - 1, DEFAULT_UDP_IP_ADDR);
    }
}

int pirate_udp_socket_parse_param(char *str, pirate_udp_socket_param_t *param) {
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
        if (strncmp("iov_len", key, strlen("iov_len")) == 0) {
            param->iov_len = strtol(val, NULL, 10);
        } else if (strncmp("buffer_size", key, strlen("buffer_size")) == 0) {
            param->buffer_size = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_udp_socket_get_channel_description(const pirate_udp_socket_param_t *param, char *desc, int len) {
    return snprintf(desc, len - 1, "udp_socket,%s,%u,iov_len=%u,buffer_size=%u", param->addr,
                    param->port, param->iov_len, param->buffer_size);
}

static int udp_socket_reader_open(pirate_udp_socket_param_t *param, udp_socket_ctx *ctx) {
    int err, rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(param->addr);
    addr.sin_port = htons(param->port);

    int enable = 1;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_RCVBUF,
                        &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(ctx->sock);
            ctx->sock = -1;
            errno = err;
            return rv;
        }
    }

    rv = bind(ctx->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    return 0;
}

static int udp_socket_writer_open(pirate_udp_socket_param_t *param, udp_socket_ctx *ctx) {
    int err, rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF,
                        &param->buffer_size,
                        sizeof(param->buffer_size));
        if (rv < 0) {
            err = errno;
            close(ctx->sock);
            ctx->sock = -1;
            errno = err;
            return rv;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(param->addr);
    addr.sin_port = htons(param->port);
    rv = connect(ctx->sock, (const struct sockaddr*) &addr, sizeof(addr));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    return 0;
}

int pirate_udp_socket_open(int flags, pirate_udp_socket_param_t *param, udp_socket_ctx *ctx) {
    int rv = -1;
    int access = flags & O_ACCMODE;

    pirate_udp_socket_init_param(param);
    if (param->port <= 0) {
        errno = EINVAL;
        return -1;
    }
    if (access == O_RDONLY) {
        rv = udp_socket_reader_open(param, ctx);
    } else {
        rv = udp_socket_writer_open(param, ctx);
    }

    return rv;
}

int pirate_udp_socket_close(udp_socket_ctx *ctx) {
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

ssize_t pirate_udp_socket_read(const pirate_udp_socket_param_t *param, udp_socket_ctx *ctx, void *buf, size_t count) {
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    if (param->iov_len > 0) {
        struct mmsghdr msgvec[PIRATE_IOV_MAX];
        struct iovec iov[PIRATE_IOV_MAX];
        int vlen = pirate_make_msgvec(buf, count, param->iov_len,
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

ssize_t pirate_udp_socket_write(const pirate_udp_socket_param_t *param, udp_socket_ctx *ctx, const void *buf, size_t count) {
    int err;
    ssize_t rv;

    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    if (param->iov_len > 0) {
        struct mmsghdr msgvec[PIRATE_IOV_MAX];
        struct iovec iov[PIRATE_IOV_MAX];
        int vlen = pirate_make_msgvec((void *)buf, count, param->iov_len,
            msgvec, iov);
        int wr_bytes = 0;

        int mmsgrv = sendmmsg(ctx->sock, msgvec, vlen, 0);
        if (mmsgrv < 0) {
            return mmsgrv;
        }

        for (int i = 0; i < mmsgrv; i++) {
            wr_bytes += iov[i].iov_len;
        }
        
        return wr_bytes;
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
