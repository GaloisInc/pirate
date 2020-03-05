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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pirate_common.h"
#include "mercury.h"

#pragma pack(4)
typedef struct {
    uint32_t session_tag;
    uint32_t message_tag;
    uint32_t message_tlv;
    uint32_t data_tag;
    uint32_t data_len;
    uint64_t master_ts;
    uint64_t slave_ts;
} mercury_header_t;
#pragma pack()

static int mercury_message_pack(void *buf, const void *data, uint32_t mtu,
                                    const mercury_header_t *hdr) {
    mercury_header_t *msg_hdr = (mercury_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(mercury_header_t);

    if (hdr->data_len > (mtu - sizeof(mercury_header_t))) {
        errno = ENOBUFS;
        return -1;
    }

    memset(buf, 0, mtu);

    msg_hdr->session_tag = htobe32(hdr->session_tag);
    msg_hdr->message_tag = htobe32(hdr->message_tag);
    msg_hdr->message_tlv = htobe32(hdr->message_tlv);
    msg_hdr->data_tag    = htobe32(hdr->data_tag);
    msg_hdr->data_len    = htobe32(hdr->data_len);
    msg_hdr->master_ts   = htobe64(0x4d41535445525453);
    msg_hdr->slave_ts    = htobe64(0x534c4156455f5453);

    memcpy(msg_data, data, hdr->data_len);
    return 0;
}

static int mercury_message_unpack(const void *buf, void *data, uint32_t mtu,
                                size_t data_buf_len, mercury_header_t *hdr) {
    const mercury_header_t *msg_hdr = (mercury_header_t *)buf;
    const uint8_t *msg_data = (uint8_t *)buf + sizeof(mercury_header_t);

    hdr->session_tag = be32toh(msg_hdr->session_tag);
    hdr->message_tag = be32toh(msg_hdr->message_tag);
    hdr->message_tlv = be32toh(msg_hdr->message_tlv);
    hdr->data_tag    = be32toh(msg_hdr->data_tag);
    hdr->data_len    = be32toh(msg_hdr->data_len);
    hdr->master_ts   = be64toh(msg_hdr->master_ts);
    hdr->slave_ts    = be64toh(msg_hdr->slave_ts);

    if ((hdr->data_len > data_buf_len) ||
        (hdr->data_len > (mtu - sizeof(mercury_header_t)))) {
        errno = ENOBUFS;
        return -1;
    }

    memcpy(data, msg_data, hdr->data_len);
    return 0;
}

void pirate_mercury_init_param(int gd, pirate_mercury_param_t *param) {
    if (strnlen(param->path, 1) == 0) {
        snprintf(param->path, PIRATE_LEN_NAME - 1, PIRATE_MERCURY_NAME_FMT, gd);
    }
    if (param->mtu == 0) {
        param->mtu = PIRATE_MERCURY_DEFAULT_MTU;
    }
}

int pirate_mercury_parse_param(char *str, pirate_mercury_param_t *param) {
    char *ptr = NULL;

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) ||
        (strcmp(ptr, "mercury") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->path, ptr, sizeof(param->path));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->mtu = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_mercury_open(int gd, int flags, pirate_mercury_param_t *param, mercury_ctx *ctx) {
    int rv = -1;


    pirate_mercury_init_param(gd, param);
    // Current implementation uses pipe as a loopback
    // Remove pipe creation once it is no longer needed
    rv = mkfifo(param->path, 0660);
    if (rv == -1) {
        if (errno == EEXIST) {
            errno = 0;
        } else {
            return -1;
        }
    }

    ctx->buf = (uint8_t *) malloc(param->mtu);
    if (ctx->buf == NULL) {
        return -1;
    }

    if ((ctx->fd = open(param->path, flags)) < 0) {
        return -1;
    }

    return gd;
}

int pirate_mercury_close(mercury_ctx *ctx) {
    int rv = -1;

    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
    }

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_mercury_read(pirate_mercury_param_t *param, mercury_ctx *ctx, void *buf, size_t count) {
    int rv;
    mercury_header_t hdr = { 0 };

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = read(ctx->fd, ctx->buf, param->mtu);
    if (rv < 0) {
        return -1;
    } else if (rv < ((int)sizeof(mercury_header_t))) {
        errno = EIO;
        return -1;
    }

    if (mercury_message_unpack(ctx->buf, buf, param->mtu, count, &hdr) != 0) {
        errno = ENOMSG;
        return -1;
    }

    return hdr.data_len;
}

ssize_t pirate_mercury_write(pirate_mercury_param_t *param, mercury_ctx *ctx, const void *buf, size_t count) {
    mercury_header_t hdr = {
        .session_tag = 1,
        .message_tag = 1,
        .message_tlv = 1,
        .data_tag = 1,
        .data_len = count
    };

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if (mercury_message_pack(ctx->buf, buf, param->mtu, &hdr) != 0) {
        errno = ENOMSG;
        return -1;
    }

    if (write(ctx->fd, ctx->buf, param->mtu) != param->mtu) {
        return -1;
    }

    return count;
}
