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

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include "device.h"
#include "pirate_common.h"

int pirate_device_parse_param(char *str, pirate_device_param_t *param) {
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "device") != 0)) {
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
        if (strncmp("iov_len", key, strlen("iov_len")) == 0) {
            param->iov_len = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_device_get_channel_description(const pirate_device_param_t *param, char *desc, int len) {
    return snprintf(desc, len - 1, "device,%s,iov_len=%u", param->path, param->iov_len);
}

int pirate_device_open(int flags, pirate_device_param_t *param, device_ctx *ctx) {
    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    if ((ctx->fd = open(param->path, flags)) < 0) {
        return -1;
    }
    
    return 0;
}

int pirate_device_close(device_ctx *ctx) {
    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}


ssize_t pirate_device_read(const pirate_device_param_t *param, device_ctx *ctx, void *buf, size_t count) {
    return pirate_fd_read(ctx->fd, buf, count, param->iov_len);
}

ssize_t pirate_device_write(const pirate_device_param_t *param, device_ctx *ctx, const void *buf, size_t count) {
    return pirate_fd_write(ctx->fd, buf, count, param->iov_len);
}
