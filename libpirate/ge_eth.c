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

#pragma pack(1)
typedef struct {
    uint32_t message_id;
    uint16_t data_len;
    uint16_t crc16;
} ge_header_t;
#pragma pack()

static const unsigned int crc16_ccitt_table_reverse[256] =
{
 0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
 0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
 0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
 0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
 0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
 0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
 0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
 0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
 0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
 0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
 0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
 0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
 0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
 0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
 0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
 0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
 0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
 0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
 0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
 0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
 0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
 0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
 0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
 0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
 0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
 0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
 0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
 0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
 0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
 0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
 0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
 0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};

static uint16_t crc16_reflected(const uint8_t *buf, uint16_t len,
                            uint16_t crc_in, const unsigned int table[])
{
    unsigned int crc16 = (unsigned int) crc_in;

    while( len-- != 0 )
       crc16 = table[(crc16 ^ *buf++) & 0xff] ^ (crc16 >> 8);

    return (uint16_t) crc16;
}

static const uint16_t crc16_ccitt_start = 0xFFFF;
static const uint16_t crc16_ccitt_xorout = 0xFFFF;

uint16_t pirate_ge_eth_crc16(const uint8_t *data, uint16_t len) {
    return crc16_reflected(data, len, crc16_ccitt_start, crc16_ccitt_table_reverse)
        ^ crc16_ccitt_xorout;
}

static ssize_t ge_message_pack(void *buf, const void *data, uint32_t count,
    const pirate_ge_eth_param_t *param) {
    ge_header_t *msg_hdr = (ge_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);

    if (count > (param->mtu - sizeof(ge_header_t))) {
        errno = EMSGSIZE;
        return -1;
    }

    msg_hdr->message_id = htobe32(param->message_id);
    msg_hdr->data_len = htobe16(count);
    msg_hdr->crc16 = htobe16(pirate_ge_eth_crc16(buf, sizeof(ge_header_t)-sizeof(uint16_t)));

    memcpy(msg_data, data, count);
    return sizeof(ge_header_t) + count;
}

static ssize_t ge_message_unpack(const void *buf, void *data,
                                size_t data_buf_len, ge_header_t *hdr) {
    const ge_header_t *msg_hdr = (ge_header_t *)buf;
    const uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);
    size_t copy_len;

    hdr->message_id = be32toh(msg_hdr->message_id);
    hdr->data_len   = be16toh(msg_hdr->data_len);
    hdr->crc16      = be16toh(msg_hdr->crc16);

    copy_len = MIN(hdr->data_len, data_buf_len);

    memcpy(data, msg_data, copy_len);
    return copy_len;
}

static void pirate_ge_eth_init_param(pirate_ge_eth_param_t *param) {
    if (param->mtu == 0) {
        param->mtu = PIRATE_DEFAULT_GE_ETH_MTU;
    }
    if (strnlen(param->reader_addr, 1) == 0) {
        strncpy(param->reader_addr, "0.0.0.0", sizeof(param->reader_addr) - 1);
    }
    if (strnlen(param->writer_addr, 1) == 0) {
        strncpy(param->writer_addr, "0.0.0.0", sizeof(param->writer_addr) - 1);
    }
}

int pirate_ge_eth_parse_param(char *str, void *_param) {
    pirate_ge_eth_param_t *param = (pirate_ge_eth_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "ge_eth") != 0)) {
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

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->message_id = strtol(ptr, NULL, 10);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_ge_eth_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_ge_eth_param_t *param = (const pirate_ge_eth_param_t *)_param;
    char mtu_str[32];

    mtu_str[0] = 0;
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    return snprintf(desc, len, "ge_eth,%s,%u,%s,%u,%u%s",
        param->reader_addr, param->reader_port,
        param->writer_addr, param->writer_port,
        param->message_id, mtu_str);
}

static int ge_eth_reader_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int err, rv;
    struct sockaddr_in src_addr, dest_addr;
    int nonblock = ctx->flags & O_NONBLOCK;

    ctx->sock = socket(AF_INET, SOCK_DGRAM | nonblock, 0);
    if (ctx->sock < 0) {
        return -1;
    }

    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = inet_addr(param->reader_addr);
    src_addr.sin_port = htons(param->reader_port);

    memset(&dest_addr, 0, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(param->writer_addr);
    dest_addr.sin_port = htons(param->writer_port);

    int enable = 1;
    rv = setsockopt(ctx->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return -1;
    }

    rv = bind(ctx->sock, (struct sockaddr *)&src_addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return -1;
    }

    rv = connect(ctx->sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return -1;
    }

    return ctx->sock;
}

static int ge_eth_writer_open(pirate_ge_eth_param_t *param, ge_eth_ctx *ctx) {
    int err, rv;
    struct sockaddr_in src_addr, dest_addr;
    int nonblock = ctx->flags & O_NONBLOCK;

    ctx->sock = socket(AF_INET, SOCK_DGRAM | nonblock, 0);
    if (ctx->sock < 0) {
        return -1;
    }

    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = inet_addr(param->writer_addr);
    src_addr.sin_port = htons(param->writer_port);

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(param->reader_addr);
    dest_addr.sin_port = htons(param->reader_port);

    rv = bind(ctx->sock, (struct sockaddr *)&src_addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        errno = err;
        return -1;
    }
    rv = connect(ctx->sock, (const struct sockaddr*) &dest_addr, sizeof(struct sockaddr_in));
    if (rv < 0) {
        err = errno;
        close(ctx->sock);
        ctx->sock = -1;
        errno = err;
        return -1;
    }

    return ctx->sock;
}

int pirate_ge_eth_open(void *_param, void *_ctx) {
    pirate_ge_eth_param_t *param = (pirate_ge_eth_param_t *)_param;
    ge_eth_ctx *ctx = (ge_eth_ctx *)_ctx;
    int rv = -1;
    int access = ctx->flags & O_ACCMODE;

    pirate_ge_eth_init_param(param);
    if (param->reader_port <= 0) {
        errno = EINVAL;
        return -1;
    }
    if (param->writer_port < 0) {
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

int pirate_ge_eth_close(void *_ctx) {
    ge_eth_ctx *ctx = (ge_eth_ctx *)_ctx;
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

ssize_t pirate_ge_eth_read(const void *_param, void *_ctx, void *buf, size_t count) {
    const pirate_ge_eth_param_t *param = (const pirate_ge_eth_param_t *)_param;
    ge_eth_ctx *ctx = (ge_eth_ctx *)_ctx;
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

    return ge_message_unpack(ctx->buf, buf, count, &hdr);
}

ssize_t pirate_ge_eth_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_ge_eth_param_t *param = (const pirate_ge_eth_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        mtu = PIRATE_DEFAULT_GE_ETH_MTU;
    }
    if (mtu < sizeof(ge_header_t)) {
        errno = EINVAL;
        return -1;
    }
    return mtu - sizeof(ge_header_t);
}

ssize_t pirate_ge_eth_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_ge_eth_param_t *param = (const pirate_ge_eth_param_t *)_param;
    ge_eth_ctx *ctx = (ge_eth_ctx *)_ctx;
    ssize_t rv, wr_len;
    int err;

    if ((wr_len = ge_message_pack(ctx->buf, buf, count, param)) < 0) {
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
