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

#include "libpirate.h"
#include "gaps_packet.h"
#include <poll.h>


static int gaps_read_len(int gd, void *buf, size_t len) {
    do {
        ssize_t rd_len = pirate_read(gd, buf, len);
        if (rd_len == -1) {
            return -1;
        }

        buf = (uint8_t *)buf + rd_len;
        len -= rd_len;
    } while (len > 0);

    return 0;
}


static int gaps_write_len(int gd, void *buf, ssize_t len) {
    do {
        ssize_t wr_len = pirate_write(gd, buf, len);
        if (wr_len == -1) {
            return -1;
        }

        buf = (uint8_t *)buf + wr_len;
        len -= wr_len;
    } while (len > 0);

    return 0;
}

int gaps_packet_poll(int gd) {
    struct pollfd fds[1];
    int fd = pirate_get_fd(gd);
    if (fd == -1) {
        // No file descriptor so skip the polling
        return 1;
    }
    fds[0].fd = fd;
    fds[0].events = POLLIN;
    // If the timeout is too short then the fake requests
    // in the proxy queue will cause this poll to timeout.
    return poll(fds, 1, 3000);
}

ssize_t gaps_packet_read(int gd, void *buf, uint32_t buf_len) {
    int rv = gaps_packet_poll(gd);
    if (rv < 0) {
        return -1;
    }

    if (rv == 0) {
        return 0;
    }

    /* Read the length */
    uint32_t len = 0;
    if ((gaps_read_len(gd, &len, sizeof(len)) == -1) || (len > buf_len)) {
        return -1;
    }

    /* Read the packet */
    if (gaps_read_len(gd, buf, len) == -1) {
        return -1;
    }

    return len;
}


int gaps_packet_write(int gd, void *buf, size_t len) {
    /* Write the length */
    uint32_t pkt_len = len;
    if (gaps_write_len(gd, &pkt_len, sizeof(pkt_len)) == -1) {
        return -1;
    }

    /* Write the packet */
    return gaps_write_len(gd, buf, len); 
}
