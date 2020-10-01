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

#include <fcntl.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#define O_NONBLOCK	  04000

#include "pirate_common.h"
#include "udp_socket.h"

static void pirate_udp_socket_init_param(pirate_udp_socket_param_t *param) {
    if (param->mtu == 0) {
        param->mtu = PIRATE_DEFAULT_UDP_PACKET_SIZE;
    }
}

static short strtos(char const* input, char** endptr, int radix) {
    long lval = strtol(input, endptr, radix);
    return lval & 0xFFFF;
}

static int strtoi(char const* input, char** endptr, int radix) {
    long lval = strtol(input, endptr, radix);
    return lval & 0xFFFFFFFF;
}

int pirate_udp_socket_parse_param(char *str, void *_param) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_s(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strncmp(ptr, "udp_socket", strlen("udp_socket")) != 0)) {
        return -1;
    }

    if ((ptr = strtok_s(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    strncpy_s(param->reader_addr, sizeof(param->reader_addr) - 1, ptr, _TRUNCATE);

    if ((ptr = strtok_s(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    param->reader_port = strtos(ptr, NULL, 10);

    if ((ptr = strtok_s(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    strncpy_s(param->writer_addr, sizeof(param->writer_addr) - 1, ptr, _TRUNCATE);

    if ((ptr = strtok_s(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    param->writer_port = strtos(ptr, NULL, 10);

    while ((ptr = strtok_s(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        }
        else if (rv == 0) {
            continue;
        }
        if (strncmp("buffer_size", key, strlen("buffer_size")) == 0) {
            param->buffer_size = strtoi(val, NULL, 10);
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtoi(val, NULL, 10);
        } else {
            SetLastError(WSAEINVAL);
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
        _snprintf_s(mtu_str, 32, _TRUNCATE, ",mtu=%u", param->mtu);
    }
    if (param->buffer_size != 0) {
        _snprintf_s(buffer_size_str, 32, _TRUNCATE, ",buffer_size=%u", param->buffer_size);
    }
    return _snprintf_s(desc, len, _TRUNCATE, "udp_socket,%s,%u,%s,%u%s%s",
        param->reader_addr, param->reader_port,
        param->writer_addr, param->writer_port,
        buffer_size_str, mtu_str);
}

static int udp_socket_reader_open(pirate_udp_socket_param_t *param, udp_socket_ctx *ctx) {
    int err, rv;
    struct sockaddr_in addr;
    int nonblock = ctx->flags & O_NONBLOCK;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock == INVALID_SOCKET) {
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    InetPton(AF_INET, param->reader_addr, &addr.sin_addr.s_addr);
    addr.sin_port = htons(param->reader_port);

    int enable = 1;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEADDR, (char*) &enable, sizeof(int));
    if (rv == SOCKET_ERROR) {
        err = GetLastError();
        closesocket(ctx->sock);
        ctx->sock = INVALID_SOCKET;
        SetLastError(err);
        return rv;
    }

    if (nonblock) {
        rv = ioctlsocket(ctx->sock, FIONBIO, &enable);
        if (rv == SOCKET_ERROR) {
            err = GetLastError();
            closesocket(ctx->sock);
            ctx->sock = INVALID_SOCKET;
            SetLastError(err);
            return rv;
        }
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_RCVBUF,
            (char*) &param->buffer_size,
            sizeof(param->buffer_size));
        if (rv == SOCKET_ERROR) {
            err = GetLastError();
            closesocket(ctx->sock);
            ctx->sock = INVALID_SOCKET;
            SetLastError(err);
            return rv;
        }
    }

    rv = bind(ctx->sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (rv == SOCKET_ERROR) {
        err = GetLastError();
        closesocket(ctx->sock);
        ctx->sock = INVALID_SOCKET;
        SetLastError(err);
        return rv;
    }

    return 0;
}

static int udp_socket_writer_open(pirate_udp_socket_param_t *param, udp_socket_ctx *ctx) {
    int err, rv;
    struct sockaddr_in addr;
    int nonblock = ctx->flags & O_NONBLOCK;
    int enable = 1;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock == INVALID_SOCKET) {
        return -1;
    }

    if (param->buffer_size > 0) {
        rv = setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF,
            (char*) &param->buffer_size,
            sizeof(param->buffer_size));
        if (rv == SOCKET_ERROR) {
            err = GetLastError();
            closesocket(ctx->sock);
            ctx->sock = INVALID_SOCKET;
            SetLastError(err);
            return rv;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    InetPton(AF_INET, param->reader_addr, &addr.sin_addr.s_addr);
    addr.sin_port = htons(param->reader_port);
    rv = connect(ctx->sock, (const struct sockaddr*) &addr, sizeof(addr));
    if (rv == SOCKET_ERROR) {
        err = GetLastError();
        closesocket(ctx->sock);
        ctx->sock = INVALID_SOCKET;
        SetLastError(err);
        return rv;
    }

    if (nonblock) {
        rv = ioctlsocket(ctx->sock, FIONBIO, &enable);
        if (rv == SOCKET_ERROR) {
            err = GetLastError();
            closesocket(ctx->sock);
            ctx->sock = INVALID_SOCKET;
            SetLastError(err);
            return rv;
        }
    }

    return 0;
}

int pirate_udp_socket_open(void *_param, void *_ctx) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;

    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_udp_socket_init_param(param);
    if (param->reader_port <= 0) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    if (strncmp(param->writer_addr, "0.0.0.0", strlen("0.0.0.0")) != 0) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    if (param->writer_port != 0) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    if (param->mtu > PIRATE_DEFAULT_UDP_PACKET_SIZE) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    rv = WSAStartup(MAKEWORD(2, 2), &ctx->wsaData);
    if (rv != 0) {
        SetLastError(rv);
        return -1;
    }
    if (access == _O_RDONLY) {
        rv = udp_socket_reader_open(param, ctx);
    } else {
        rv = udp_socket_writer_open(param, ctx);
    }

    return rv;
}

int pirate_udp_socket_close(void *_ctx) {
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;
    int err, rv = -1;

    if (ctx->sock == INVALID_SOCKET) {
        SetLastError(WSAEBADF);
        return -1;
    }

    err = GetLastError();
    shutdown(ctx->sock, SD_BOTH);
    SetLastError(err);

    rv = closesocket(ctx->sock);
    ctx->sock = INVALID_SOCKET;

    err = GetLastError();
    WSACleanup();
    SetLastError(err);

    return rv;
}

SSIZE_T pirate_udp_socket_read(const void *_param, void *_ctx, void *buf, size_t count) {
    SSIZE_T rv;
    (void) _param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;

    if (ctx->sock <= 0) {
        SetLastError(WSAEBADF);
        return -1;
    }

    rv = recv(ctx->sock, buf, count & 0xFFFFFFFF, 0);
    if (GetLastError() == WSAEMSGSIZE) {
        // On Windows if the recv buffer is smaller than the datagram
        // then the recv buffer is filled with the truncated datagram
        // but the error return value is set. We want the POSIX behavior.
        SetLastError(0);
        rv = count;
    }
    return rv;
}

SSIZE_T pirate_udp_socket_write_mtu(const void *_param) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        mtu = PIRATE_DEFAULT_UDP_PACKET_SIZE;
    }
    if (mtu > PIRATE_DEFAULT_UDP_PACKET_SIZE) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    // 8 byte UDP header and 20 byte IP header
    if (mtu < 28) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    return mtu - 28;
}

SSIZE_T pirate_udp_socket_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    pirate_udp_socket_param_t *param = (pirate_udp_socket_param_t *)_param;
    udp_socket_ctx *ctx = (udp_socket_ctx *)_ctx;
    int err;
    SSIZE_T rv;
    size_t write_mtu = pirate_udp_socket_write_mtu(param);

    if (ctx->sock == INVALID_SOCKET) {
        SetLastError(WSAEBADF);
        return -1;
    }
    if ((write_mtu > 0) && (count > write_mtu)) {
        SetLastError(WSAEMSGSIZE);
        return -1;
    }
    err = GetLastError();
    rv = send(ctx->sock, buf, count & 0xFFFFFFFF, 0);
    if ((rv < 0) && (GetLastError() == WSAECONNREFUSED)) {
        // TODO create a counter of undelivered messages
        SetLastError(err);
        rv = send(ctx->sock, buf, count & 0xFFFFFFFF, 0);
    }
    return rv;
}
