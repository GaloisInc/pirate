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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <string.h>
#include "pirate_common.h"
#include "ge_eth.h"

#define CRC16 0x8005

#pragma pack(1)
typedef struct {
    uint32_t message_id;
    uint16_t data_len;
    uint16_t crc16;
} ge_header_t;
#pragma pack()

static uint16_t crc16(const uint8_t *data, uint16_t len) {
    uint16_t out = 0;
    int bits_read = 0;
    int bit_flag = 0;

    while(len > 0) {
        bit_flag = out >> 15;

        out <<= 1;
        out |= (*data >> bits_read) & 1;

        bits_read++;
        if(bits_read > 7) {
            bits_read = 0;
            data++;
            len--;
        }

        if (bit_flag) {
            out ^= CRC16;
        }
    }

    for (uint16_t i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag) {
            out ^= CRC16;
        }
    }

    uint16_t crc = 0;
    uint16_t j = 0x0001;
    for (uint16_t i = 0x8000; i != 0; i >>=1, j <<= 1) {
        if (i & out) {
            crc |= j;
        }
    }

    return crc;
}

static ssize_t ge_message_pack(void *buf, const void *data, uint32_t mtu,
                                const ge_header_t *hdr) {
    ge_header_t *msg_hdr = (ge_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);

    if (hdr->data_len > (mtu - sizeof(ge_header_t))) {
        errno = ENOBUFS;
        return -1;
    }

    msg_hdr->message_id = htobe32(hdr->message_id);
    msg_hdr->data_len = htobe16(hdr->data_len);
    msg_hdr->crc16 = htobe16(crc16(buf, sizeof(ge_header_t)-sizeof(uint16_t)));

    memcpy(msg_data, data, hdr->data_len);
    return hdr->data_len + sizeof(ge_header_t);
}

static int ge_message_unpack(const void *buf, void *data, uint32_t mtu,
                                size_t data_buf_len, ge_header_t *hdr) {
    const ge_header_t *msg_hdr = (ge_header_t *)buf;
    const uint8_t *msg_data = (uint8_t *)buf + sizeof(msg_hdr);

    hdr->message_id = be32toh(msg_hdr->message_id);
    hdr->data_len   = be16toh(msg_hdr->data_len);
    hdr->crc16      = be16toh(msg_hdr->crc16);

    if (hdr->crc16 != crc16(buf, sizeof(ge_header_t) - sizeof(uint16_t))) {
        errno = EBADMSG;
        return -1;
    }

    if ((hdr->data_len > data_buf_len) ||
        (hdr->data_len > (mtu - sizeof(ge_header_t)))) {
        errno = ENOBUFS;
        return -1;
    }

    memcpy(data, msg_data, hdr->data_len);
    return 0;
}

static void pirate_ge_eth_init_param(int gd, pirate_ge_eth_param_t *param) {
    if (strnlen(param->addr, 1) == 0) {
        snprintf(param->addr, sizeof(param->addr) - 1, DEFAULT_GE_ETH_IP_ADDR);
    }
    if (param->port == 0) {
        param->port = DEFAULT_GE_ETH_IP_PORT + gd;
    }
    if (param->mtu == 0) {
        param->mtu = DEFAULT_GE_ETH_MTU;
    }
}

int pirate_ge_eth_parse_param(char *str, pirate_ge_eth_param_t *param) {
    char *ptr = NULL;

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "ge_eth") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->addr, ptr, sizeof(param->addr));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->port = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->mtu = strtol(ptr, NULL, 10);
    }

    return 0;
}

static int ge_eth_reader_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(param->port);

    int enable = 1;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        int err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
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

static int ge_eth_writer_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int rv;
    struct sockaddr_in addr;

    ctx->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock < 0) {
        return ctx->sock;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(param->addr);
    addr.sin_port = htons(param->port);
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

int pirate_ge_eth_open(int gd, int flags, pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int rv = -1;

    pirate_ge_eth_init_param(gd, param);
    ctx->buf = (uint8_t *) malloc(param->mtu);
    if (ctx->buf == NULL) {
        return -1;
    }

    if (flags == O_RDONLY) {
        rv = ge_eth_reader_open(param, ctx);
    } else if (flags == O_WRONLY) {
        rv = ge_eth_writer_open(param, ctx);
    }

    return rv == 0 ? gd : rv;
}

int pirate_ge_eth_close(ge_eth_ctx *ctx) {
    int rv = -1;

    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
    }

    if (ctx->sock <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->sock);
    ctx->sock = -1;

    return rv;
}

ssize_t pirate_ge_eth_read(const pirate_ge_eth_param_t *param, ge_eth_ctx *ctx, void *buf, size_t count) {
    ssize_t rd_size;
    ge_header_t hdr;

    memset(&hdr, 0, sizeof(ge_header_t));
    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    rd_size = recv(ctx->sock, ctx->buf, param->mtu, 0);
    if (rd_size < 0) {
        return 0;
    }

    if (ge_message_unpack(ctx->buf, buf, param->mtu, count, &hdr) != 0) {
        return -1;
    }

    return hdr.data_len;
}

ssize_t pirate_ge_eth_write(const pirate_ge_eth_param_t *param, ge_eth_ctx *ctx, const void *buf, size_t count) {
    ge_header_t hdr = {
        .message_id = 1,
        .data_len = count,
        .crc16 = 0x0000
    };
    ssize_t wr_len = -1;

    if ((wr_len = ge_message_pack(ctx->buf, buf, param->mtu, &hdr)) < 0) {
        errno = ENOMSG;
        return -1;
    }

    if (send(ctx->sock, ctx->buf, wr_len, 0) != wr_len) {
        return -1;
    }

    return count;
}
