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
        param->baud = PIRATE_SERIAL_DEFAULT_BAUD;
    }
    if (param->max_tx == 0) {
        param->max_tx = PIRATE_SERIAL_DEFAULT_MAX_TX;
    }
}

int pirate_serial_parse_param(char *str, void *_param) {
    pirate_serial_param_t *param = (pirate_serial_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "serial") != 0)) {
        return -1;
    }

    pirate_serial_init_param(param);

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
            } else if (strncmp("460800", val, strlen("460800")) == 0) {
                param->baud = B460800;
            } else {
                errno = EINVAL;
                return -1;
            }
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else if (strncmp("max_tx_size", key, strlen("max_tx_size")) == 0) {
            param->max_tx = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_serial_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_serial_param_t *param = (const pirate_serial_param_t *)_param;
    char baud_str[32];
    char mtu_str[32];
    char max_tx_str[32];
    const char *baud_val = NULL;
    
    switch (param->baud) {
    case B4800:   baud_val = "4800";   break;
    case B9600:   baud_val = "9600";   break;
    case B19200:  baud_val = "19200";  break;
    case B38400:  baud_val = "38400";  break;
    case B57600:  baud_val = "57600";  break;
    case B115200: baud_val = "115200"; break;
    case B230400: baud_val = "230400"; break;
    case B460800: baud_val = "460800"; break;
    default:
        return -1;
    }

    baud_str[0] = 0;
    mtu_str[0] = 0;
    max_tx_str[0] = 0;
    if ((param->baud != 0) && (param->baud != PIRATE_SERIAL_DEFAULT_BAUD)) {
        snprintf(baud_str, 32, ",baud=%s", baud_val);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    if ((param->max_tx != 0) && (param->max_tx != PIRATE_SERIAL_DEFAULT_MAX_TX)) {
        snprintf(max_tx_str, 32, ",max_tx_size=%u", param->max_tx);
    }
    return snprintf(desc, len, "serial,%s%s%s%s", param->path, baud_str, mtu_str, max_tx_str);
}

int pirate_serial_open(void *_param, void *_ctx, int *server_fdp) {
    (void) server_fdp;
    pirate_serial_param_t *param = (pirate_serial_param_t *)_param;
    serial_ctx *ctx = (serial_ctx *)_ctx;
    struct termios attr;

    pirate_serial_init_param(param);
    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    ctx->fd = open(param->path, ctx->flags | O_NOCTTY);
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

int pirate_serial_close(void *_ctx) {
    serial_ctx *ctx = (serial_ctx *)_ctx;
    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

static ssize_t serial_do_read(serial_ctx *ctx, uint8_t *buf, size_t count) {
    size_t rx = 0;
    ssize_t rv;
    int fd = ctx->fd;

    while (rx < count) {
        rv = read(fd, buf + rx, count - rx);
        if (rv < 0) {
            return rv;
        }
        rx += rv;
    }
    return rx;
}

static ssize_t serial_do_write(const pirate_serial_param_t *param, serial_ctx *ctx, const void *buf, size_t count) {
    const uint8_t *wr_buf = (const uint8_t *) buf;
    size_t remain = count;
    do {
        int rv;
        uint32_t tx_buf_bytes = 0;
        size_t wr_len = MIN(remain, param->max_tx);
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

ssize_t pirate_serial_read(const void *_param, void *_ctx, void *buf, size_t count) {
    serial_ctx *ctx = (serial_ctx *)_ctx;
    (void) _param;
    pirate_header_t header;
    ssize_t rv;
    size_t packet_count;

    rv = serial_do_read(ctx, (uint8_t*) &header, sizeof(header));
    if (rv < 0) {
        return rv;
    }
    packet_count = ntohl(header.count);
    count = MIN(count, packet_count);
    rv = serial_do_read(ctx, buf, count);
    if (rv < 0) {
        return rv;
    }
    if (count < packet_count) {
        // slow path
        uint8_t *temp = malloc(packet_count - count);
        rv = serial_do_read(ctx, temp, packet_count - count);
        free(temp);
        if (rv < 0) {
            return rv;
        }
    }
    return count;
}

ssize_t pirate_serial_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_serial_param_t *param = (const pirate_serial_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        return 0;
    }
    if (mtu < sizeof(pirate_header_t)) {
        errno = EINVAL;
        return -1;
    }
    return mtu - sizeof(pirate_header_t);
}

ssize_t pirate_serial_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_serial_param_t *param = (const pirate_serial_param_t *)_param;
    serial_ctx *ctx = (serial_ctx *)_ctx;
    pirate_header_t header;
    header.count = htonl(count);
    ssize_t rv;
    size_t mtu = pirate_serial_write_mtu(param, ctx);

    if ((mtu > 0) && (count > mtu)) {
        errno = EMSGSIZE;
        return -1;
    }

    rv = serial_do_write(param, ctx, &header, sizeof(header));
    if (rv < 0) {
        return rv;
    }
    rv = serial_do_write(param, ctx, buf, count);
    if (rv < 0) {
        return rv;
    }
    return count;
}
