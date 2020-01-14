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
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "primitives.h"
#include "serial.h"

static const speed_t BAUD = B230400;

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

    if (cfsetispeed(&attr, BAUD) || cfsetispeed(&attr, BAUD)) {
        return -1;
    }

    cfmakeraw(&attr);

    if (tcsetattr(fd, TCSANOW, &attr)) {
        return -1;
    }

    ch->fd = fd;
    return gd;
}
