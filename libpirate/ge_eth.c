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

static const unsigned int crc16_ccitt_table[256] =
{
 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
 0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
 0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/* two types of crcs are possible: unreflected (bits shift left) and
 * reflected (bits shift right).
 */
static uint16_t crc16_unreflected(const uint8_t *buf, uint16_t len,
                            uint16_t crc_in, const unsigned int table[])
{
    unsigned int crc16 = (unsigned int) crc_in;

    while( len-- != 0 )
        crc16 = table[((crc16 >> 8) ^ *buf++) & 0xff] ^ (crc16 << 8);

    return (uint16_t) crc16;
}

static uint16_t crc16(const uint8_t *data, uint16_t len) {
    return crc16_unreflected(data, len, 0xFFFF, crc16_ccitt_table);
}

static ssize_t ge_message_pack(void *buf, const void *data, uint32_t count,
    const pirate_ge_eth_param_t *param) {
    ge_header_t *msg_hdr = (ge_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);

    if (count > (param->mtu - sizeof(ge_header_t))) {
        errno = ENOBUFS;
        return -1;
    }

    msg_hdr->message_id = htobe32(param->message_id);
    msg_hdr->data_len = htobe16(count);
    msg_hdr->crc16 = htobe16(crc16(buf, sizeof(ge_header_t)-sizeof(uint16_t)));

    memcpy(msg_data, data, count);
    return sizeof(ge_header_t) + count;
}

static int ge_message_unpack(const void *buf, void *data,
                                size_t data_buf_len, ge_header_t *hdr,
                                const pirate_ge_eth_param_t *param) {
    const ge_header_t *msg_hdr = (ge_header_t *)buf;
    const uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);

    hdr->message_id = be32toh(msg_hdr->message_id);
    hdr->data_len   = be16toh(msg_hdr->data_len);
    hdr->crc16      = be16toh(msg_hdr->crc16);

    if (hdr->crc16 != crc16(buf, sizeof(ge_header_t) - sizeof(uint16_t))) {
        errno = EBADMSG;
        return -1;
    }

    if ((hdr->data_len > data_buf_len) ||
        (hdr->data_len > (param->mtu - sizeof(ge_header_t)))) {
        errno = ENOBUFS;
        return -1;
    }

    if (hdr->message_id != param->message_id) {
        errno = ENOMSG;
        return -1;
    }

    memcpy(data, msg_data, hdr->data_len);
    return 0;
}

static void pirate_ge_eth_init_param(pirate_ge_eth_param_t *param) {
    if (strnlen(param->addr, 1) == 0) {
        snprintf(param->addr, sizeof(param->addr) - 1, DEFAULT_GE_ETH_IP_ADDR);
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

    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->message_id = strtol(ptr, NULL, 10);

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->mtu = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_ge_eth_get_channel_description(const pirate_ge_eth_param_t *param, char *desc, int len) {
    return snprintf(desc, len - 1, "ge_eth,%s,%u,%u,%u", param->addr,
                    param->port, param->message_id, param->mtu);
}

static int ge_eth_reader_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
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

static int ge_eth_writer_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int err, rv;
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
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return rv;
    }

    return 0;
}

int pirate_ge_eth_open(int flags, pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int rv = -1;
    int access = flags & O_ACCMODE;

    pirate_ge_eth_init_param(param);
    if (param->port <= 0) {
        errno = EINVAL;
        return -1;
    }
    ctx->buf = (uint8_t *) malloc(param->mtu);
    if (ctx->buf == NULL) {
        return -1;
    }

    if (access == O_RDONLY) {
        rv = ge_eth_reader_open(param, ctx);
    } else if (access == O_WRONLY) {
        rv = ge_eth_writer_open(param, ctx);
    }

    return rv;
}

int pirate_ge_eth_close(ge_eth_ctx *ctx) {
    int err, rv = -1;

    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
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

    return rv;
}

ssize_t pirate_ge_eth_read(const pirate_ge_eth_param_t *param, ge_eth_ctx *ctx,
                            void *buf, size_t count) {
    ssize_t rd_size;
    ge_header_t hdr = { 0, 0, 0 };

    if (ctx->sock <= 0) {
        errno = EBADF;
        return -1;
    }

    rd_size = recv(ctx->sock, ctx->buf, param->mtu, 0);
    if (rd_size <= 0) {
        return rd_size;
    }

    if (ge_message_unpack(ctx->buf, buf, count, &hdr, param) != 0) {
        return -1;
    }

    return hdr.data_len;
}

ssize_t pirate_ge_eth_write(const pirate_ge_eth_param_t *param, ge_eth_ctx *ctx,
                            const void *buf, size_t count) {
    ssize_t rv, wr_len;
    int err;

    if ((wr_len = ge_message_pack(ctx->buf, buf, count, param)) < 0) {
        errno = ENOMSG;
        return -1;
    }

    err = errno;
    rv = send(ctx->sock, ctx->buf, wr_len, 0);
    if ((rv < 0) && (errno == ECONNREFUSED)) {
        // TODO create a counter of undelivered messages
        errno = err;
        rv = send(ctx->sock, ctx->buf, wr_len, 0);
    }

    if (rv != wr_len) {
        return -1;
    }

    return count;
}
