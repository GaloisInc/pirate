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
#include <endian.h>
#include <string.h>
#include "udp_socket.h"
#include "ge_eth.h"

#pragma pack(1)
typedef struct {
    uint32_t message_id;
    uint16_t data_len;
    uint16_t crc16;
} ge_header_t;
#pragma pack()

#define CRC16 0x8005

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

static ssize_t ge_message_pack(void *buf, const void *data,
                                const ge_header_t *hdr) {
    ge_header_t *msg_hdr = (ge_header_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ge_header_t);

    if (hdr->data_len > (GE_ETH_MTU - sizeof(ge_header_t))) {
        errno = ENOBUFS;
        return -1;
    }

    msg_hdr->message_id = htobe32(hdr->message_id);
    msg_hdr->data_len = htobe16(hdr->data_len);
    msg_hdr->crc16 = htobe16(crc16(buf, sizeof(ge_header_t)-sizeof(uint16_t)));

    memcpy(msg_data, data, hdr->data_len);
    return hdr->data_len + sizeof(ge_header_t);
}

static int ge_message_unpack(const void *buf, void *data, size_t data_buf_len,
                                ge_header_t *hdr) {
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
        (hdr->data_len > (GE_ETH_MTU - sizeof(ge_header_t)))) {
        errno = ENOBUFS;
        return -1;
    }

    memcpy(data, msg_data, hdr->data_len);
    return 0;
}


int pirate_ge_eth_open(int gd, int flags, pirate_channel_t *channels) {
    int fd = -1;

    if ((flags == O_WRONLY) && (channels[gd].pathname == NULL)) {
      errno = EINVAL;
      return -1;
    }

    fd = pirate_udp_socket_open(gd, flags, channels);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;

    return gd;
}

ssize_t pirate_ge_eth_read(int gd, pirate_channel_t *readers, void *buf,
                                size_t count) {
    uint8_t rd_buf[GE_ETH_MTU] = { 0 };
    ge_header_t hdr = { 0 };

    ssize_t rd_len = pirate_udp_socket_read(gd, readers, rd_buf, GE_ETH_MTU);
    if (rd_len < 0) {
        errno = EIO;
        return -1;
    }

    if (ge_message_unpack(rd_buf, buf, count, &hdr) != 0) {
        return -1;
    }

    return hdr.data_len;
}

ssize_t pirate_ge_eth_write(int gd, pirate_channel_t *writers, const void *buf,
                                size_t count) {
    ge_header_t hdr = {
        .message_id = 1,
        .data_len = count,
        .crc16 = 0x0000
    };
    uint8_t wr_buf[GE_ETH_MTU] = { 0 };
    ssize_t wr_len = -1;

    if ((wr_len = ge_message_pack(wr_buf, buf, &hdr)) < 0) {
        errno = ENOMSG;
        return -1;
    }

    if (pirate_udp_socket_write(gd, writers, wr_buf, wr_len) != wr_len) {
        errno = EIO;
        return -1;
    }

    return count;
}
