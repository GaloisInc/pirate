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

#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "primitives.h"
#include "serial.h"

static const speed_t    SERIAL_BAUD = B230400;
static const uint32_t   SERIAL_MTU  = 1024;

int pirate_serial_open(int gd, int flags, pirate_channel_t *channels) {
    pirate_channel_t *ch = &channels[gd];
    struct termios attr;

    int fd = open(ch->pathname, flags | O_NOCTTY);
    if (fd < 0) {
        return -1;
    }

    if (tcgetattr(fd, &attr) != 0) {
        return -1;
    }

    if (cfsetispeed(&attr, SERIAL_BAUD) || cfsetispeed(&attr, SERIAL_BAUD)) {
        return -1;
    }

    cfmakeraw(&attr);

    if (tcsetattr(fd, TCSANOW, &attr)) {
        return -1;
    }

    ch->fd = fd;
    return gd;
}

int pirate_serial_write(int gd, pirate_channel_t *writers, const void *buf,
    size_t count) {

    int fd = writers[gd].fd;
    const uint8_t *wr_buf = (const uint8_t *) buf;
    size_t remain = count;
    do {
        uint32_t tx_buf_bytes = 0;
        size_t wr_len = remain > SERIAL_MTU ? SERIAL_MTU : remain;
        wr_len = write(fd, wr_buf, wr_len);
        if (wr_len < 0) {
            return -1;
        }

        do {
            if (ioctl(fd, TIOCOUTQ, &tx_buf_bytes)) {
                return -1;
            }

            if (tx_buf_bytes) {
                const struct timespec one_ms = {
                    .tv_sec = 0,
                    .tv_nsec = 1000000
                };
                nanosleep(&one_ms, NULL);
            }

        } while(tx_buf_bytes > 0);

        remain -= wr_len;
        wr_buf += wr_len;
    } while(remain > 0);

    return count;
}
