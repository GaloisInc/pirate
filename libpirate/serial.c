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

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "pirate_common.h"
#include "serial.h"

static void pirate_serial_init_param(pirate_serial_param_t *param) {
    if (param->baud == 0) {
        param->baud = SERIAL_DEFAULT_BAUD;
    }
    if (param->mtu == 0) {
        param->mtu = SERIAL_DEFAULT_MTU;
    }
}

int pirate_serial_parse_param(char *str, pirate_serial_param_t *param) {
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "serial") != 0)) {
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->path, ptr, sizeof(param->path) - 1);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("baud", key, strlen("baud")) == 0) {
            if (strncmp("4800", val, strlen("4800")) == 0) {
                param->baud = B4800;
            } else if (strncmp("9600", val, strlen("9600")) == 0) {
                param->baud = B9600;
            } else if (strncmp("19200", val, strlen("19200")) == 0) {
                param->baud = B19200;
            } else if (strncmp("38400", val, strlen("38400")) == 0) {
                param->baud = B38400;
            } else if (strncmp("57600", val, strlen("57600")) == 0) {
                param->baud = B57600;
            } else if (strncmp("115200", val, strlen("115200")) == 0) {
                param->baud = B115200;
            } else if (strncmp("230400", val, strlen("230400")) == 0) {
                param->baud = B230400;
#ifdef B460800
            } else if (strncmp("460800", val, strlen("460800")) == 0) {
                param->baud = B460800;
#endif
            } else {
                errno = EINVAL;
                return -1;
            }
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_serial_get_channel_description(const pirate_serial_param_t *param, char *desc, int len) {
    const char *baud = NULL;
    
    switch (param->baud) {
    case B4800:   baud = "4800";   break;
    case B9600:   baud = "9600";   break;
    case B19200:  baud = "19200";  break;
    case B38400:  baud = "38400";  break;
    case B57600:  baud = "57600";  break;
    case B115200: baud = "115200"; break;
    case B230400: baud = "230400"; break;
#ifdef B460800
    case B460800: baud = "460800"; break;
#endif
    default:
        return -1;
    }

    return snprintf(desc, len - 1, "serial,%s,baud=%s,mtu=%u", param->path, baud,
                    param->mtu);
}

int pirate_serial_open(int flags, pirate_serial_param_t *param, serial_ctx *ctx) {
    struct termios attr;

    pirate_serial_init_param(param);
    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    ctx->fd = open(param->path, flags | O_NOCTTY);
    if (ctx->fd < 0) {
        return -1;
    }

    if (tcgetattr(ctx->fd, &attr) != 0) {
        return -1;
    }

    if (cfsetispeed(&attr, param->baud) ||
        cfsetospeed(&attr, param->baud)) {
        return -1;
    }

    cfmakeraw(&attr);

    if (tcsetattr(ctx->fd, TCSANOW, &attr)) {
        return -1;
    }

    return 0;
}

int pirate_serial_close(serial_ctx *ctx) {

    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_serial_read(const pirate_serial_param_t *param, serial_ctx *ctx, void *buf, size_t count) {
    (void) param;
    return read(ctx->fd, buf, count);
}

ssize_t pirate_serial_write(const pirate_serial_param_t *param, serial_ctx *ctx, const void *buf, size_t count) {
    const uint8_t *wr_buf = (const uint8_t *) buf;
    size_t remain = count;
    do {
        int rv;
        uint32_t tx_buf_bytes = 0;
        size_t wr_len = remain > param->mtu ? param->mtu : remain;
        rv = write(ctx->fd, wr_buf, wr_len);
        if (rv < 0) {
            return -1;
        }

        do {
            if (ioctl(ctx->fd, TIOCOUTQ, &tx_buf_bytes)) {
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

        remain -= rv;
        wr_buf += rv;
    } while(remain > 0);

    return count;
}
