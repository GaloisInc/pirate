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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <endian.h>
#include <sys/stat.h>
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

static int mercury_message_pack(void *buf, const void *data,
                                    const mercury_header_t *hdr) {
    mercury_header_t *msg_hdr = (mercury_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(mercury_header_t);

    if (hdr->data_len > (MERCURY_MTU - sizeof(mercury_header_t))) {
        errno = ENOBUFS;
        return -1;
    }

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

static int mercury_message_unpack(const void *buf, void *data,
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
        (hdr->data_len > (MERCURY_MTU - sizeof(mercury_header_t)))) {
        errno = ENOBUFS;
        return -1;
    }

    memcpy(data, msg_data, hdr->data_len);
    return 0;
}

int pirate_mercury_open(int gd, int flags, pirate_channel_t *channels) {
    pirate_channel_t *ch = &channels[gd];
    int rv = -1;

    if (ch->pathname == NULL) {
        errno = EINVAL;
        return -1;
    }

    rv = mkfifo(ch->pathname, 0660);
    if (rv == -1) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        return -1;
      }
    }

    if ((ch->fd = open(ch->pathname, flags)) < 0) {
        return -1;
    }

    return gd;
}

int pirate_mercury_close(int gd, pirate_channel_t *channels) {
    pirate_channel_t *ch = &channels[gd];
    int rv = -1;

    if (ch->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ch->fd);
    ch->fd = 0;
    return rv;
}

ssize_t pirate_mercury_read(int gd, pirate_channel_t *readers, void *buf,
                                size_t count) {
    uint8_t rd_buf[MERCURY_MTU] = { 0 };
    size_t rd_len = 0;
    mercury_header_t hdr = { 0 };
    pirate_channel_t *ch = &readers[gd];

    if (ch->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rd_len = read(ch->fd, rd_buf, MERCURY_MTU);
    if (rd_len < sizeof(mercury_header_t)) {
        errno = EIO;
        return -1;
    }

    if (mercury_message_unpack(rd_buf, buf, count, &hdr) != 0) {
        errno = ENOMSG;
        return -1;
    }

    return hdr.data_len;
}

ssize_t pirate_mercury_write(int gd, pirate_channel_t *writers, const void *buf,
                                size_t count) {
    pirate_channel_t *ch = &writers[gd];
    mercury_header_t hdr = {
        .session_tag = 1,
        .message_tag = 1,
        .message_tlv = 1,
        .data_tag = 1,
        .data_len = count
    };
    uint8_t wr_buf[MERCURY_MTU] = { 0 };

    if (ch->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if (mercury_message_pack(wr_buf, buf, &hdr) != 0) {
        errno = ENOMSG;
        return -1;
    }

    if (write(ch->fd, wr_buf, MERCURY_MTU) != MERCURY_MTU) {
        return -1;
    }

    return count;
}
