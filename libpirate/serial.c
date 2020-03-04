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

int pirate_serial_init_param(int gd, int flags, pirate_serial_param_t *param) {
    (void) flags;
    snprintf(param->path, PIRATE_LEN_NAME - 1, PIRATE_SERIAL_NAME_FMT, gd);
    param->baud = SERIAL_DEFAULT_BAUD;
    param->mtu = SERIAL_DEFAULT_MTU;
    return 0;
}

int pirate_serial_parse_param(int gd, int flags, char *str,
                                pirate_serial_param_t *param) {
    char *ptr = NULL;

    if (pirate_serial_init_param(gd, flags, param) != 0) {
        return -1;
    }

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) ||
        (strcmp(ptr, "serial") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->path, ptr, sizeof(param->path));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        if (strncmp("4800", ptr, strlen("4800")) == 0) {
            param->baud = B4800;
        } else if (strncmp("9600", ptr, strlen("9600")) == 0) {
            param->baud = B9600;
        } else if (strncmp("19200", ptr, strlen("19200")) == 0) {
            param->baud = B19200;
        } else if (strncmp("38400", ptr, strlen("38400")) == 0) {
            param->baud = B38400;
        } else if (strncmp("57600", ptr, strlen("57600")) == 0) {
            param->baud = B57600;
        } else if (strncmp("115200", ptr, strlen("115200")) == 0) {
            param->baud = B115200;
        } else if (strncmp("230400", ptr, strlen("230400")) == 0) {
            param->baud = B230400;
        } else if (strncmp("460800", ptr, strlen("460800")) == 0) {
            param->baud = B460800;
        } else {
            errno = EINVAL;
            return -1;
        }

        if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
            param->mtu = strtol(ptr, NULL, 10);
        }
    }

    return 0;
}

int pirate_serial_set_param(pirate_serial_ctx_t *ctx,
                            const pirate_serial_param_t *param) {
    if (param == NULL) {
        memset(&ctx->param, 0, sizeof(ctx->param));
    } else {
        ctx->param = *param;
    }

    return 0;
}

int pirate_serial_get_param(const pirate_serial_ctx_t *ctx,
                            pirate_serial_param_t *param) {
    *param  = ctx->param;
    return 0;
}

int pirate_serial_open(int gd, int flags, pirate_serial_ctx_t *ctx) {
    struct termios attr;

    ctx->fd = open(ctx->param.path, flags | O_NOCTTY);
    if (ctx->fd < 0) {
        return -1;
    }

    if (tcgetattr(ctx->fd, &attr) != 0) {
        return -1;
    }

    if (cfsetispeed(&attr, ctx->param.baud) ||
        cfsetispeed(&attr, ctx->param.baud)) {
        return -1;
    }

    cfmakeraw(&attr);

    if (tcsetattr(ctx->fd, TCSANOW, &attr)) {
        return -1;
    }

    return gd;
}

int pirate_serial_close(pirate_serial_ctx_t *ctx) {

    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_serial_read(pirate_serial_ctx_t *ctx, void *buf, size_t count) {
    return read(ctx->fd, buf, count);
}

ssize_t pirate_serial_write(pirate_serial_ctx_t *ctx, const void *buf,
                            size_t count) {
    const uint8_t *wr_buf = (const uint8_t *) buf;
    size_t remain = count;
    do {
        int rv;
        uint32_t tx_buf_bytes = 0;
        size_t wr_len = remain > ctx->param.mtu ? ctx->param.mtu : remain;
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
