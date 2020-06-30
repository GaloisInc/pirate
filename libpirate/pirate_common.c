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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include "libpirate.h"
#include "pirate_common.h"

static ssize_t pirate_stream_do_read(int fd, uint8_t *buf, size_t count) {
    size_t rx = 0;
    ssize_t rv;
    while (rx < count) {
        rv = read(fd, buf + rx, count - rx);
        if (rv <= 0) {
            return rv;
        }
        rx += rv;
    }
    return rx;
}

ssize_t pirate_stream_read(common_ctx *ctx, size_t min_tx, void *buf, size_t count) {
    pirate_header_t *header = (pirate_header_t*) ctx->min_tx_buf;
    int fd = ctx->fd;
    uint32_t packet_count;
    size_t rx;
    ssize_t rv;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    rv = pirate_stream_do_read(fd, ctx->min_tx_buf, min_tx);
    if (rv <= 0) {
        return rv;
    }
    packet_count = ntohl(header->count);
    count = MIN(count, packet_count);
    size_t min_tx_data = MIN(count, min_tx - sizeof(pirate_header_t));
    memcpy(buf, ctx->min_tx_buf + sizeof(pirate_header_t), min_tx_data);
    if (min_tx_data < count) {
        rv = pirate_stream_do_read(fd, ((uint8_t*) buf) + min_tx_data, count - min_tx_data);
        if (rv <= 0) {
            return rv;
        }
    }
    rx = MAX(count, min_tx - sizeof(pirate_header_t));
    if (rx < packet_count) {
        // slow path
        uint8_t *temp = malloc(packet_count - rx);
        rv = pirate_stream_do_read(fd, temp, packet_count - rx);
        free(temp);
        if (rv <= 0) {
            return rv;
        }
    }
    return count;
}

ssize_t pirate_stream_write(common_ctx *ctx, size_t min_tx, size_t write_mtu, const void *buf, size_t count) {
    pirate_header_t *header = (pirate_header_t*) ctx->min_tx_buf;
    int fd = ctx->fd;
    size_t tx = 0;
    ssize_t rv;

    if (fd < 0) {
        errno = EBADF;
        return -1;
    }
    if ((write_mtu > 0) && (count > write_mtu)) {
        errno = EMSGSIZE;
        return -1;
    }
    if (count > UINT32_MAX) {
        errno = EMSGSIZE;
        return -1;
    }
    header->count = htonl(count);
    size_t min_tx_data = MIN(count, min_tx - sizeof(pirate_header_t));
    memcpy(ctx->min_tx_buf + sizeof(pirate_header_t), buf, min_tx_data);
    while (tx < min_tx) {
        rv = write(fd, ctx->min_tx_buf + tx, min_tx - tx);
        if (rv < 0) {
            return rv;
        }
        tx += rv;
    }
    tx = min_tx_data;
    while (tx < count) {
        rv = write(fd, ((uint8_t*) buf) + tx, count - tx);
        if (rv < 0) {
            return rv;
        }
        tx += rv;
    }
    return count;
}

int pirate_parse_key_value(char **key, char **val, char *ptr, char **saveptr) {
    *key = strtok_r(ptr, KV_DELIM, saveptr);
    if (*key == NULL) {
        errno = EINVAL;
        return -1;
    }
    *val = strtok_r(NULL, KV_DELIM, saveptr);
    if (*val == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (pirate_parse_is_common_key(*key)) {
        return 0;
    }
    return 1;
}
