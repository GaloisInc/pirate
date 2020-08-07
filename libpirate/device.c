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

static void pirate_device_init_param(pirate_device_param_t *param) {
    if (param->min_tx == 0) {
        param->min_tx = PIRATE_DEFAULT_MIN_TX;
    }
}

int pirate_device_parse_param(char *str, void *_param) {
    pirate_device_param_t *param = (pirate_device_param_t *)_param;
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
        if (strncmp("min_tx_size", key, strlen("min_tx_size")) == 0) {
            param->min_tx = strtol(val, NULL, 10);
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_device_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_device_param_t *param = (const pirate_device_param_t *)_param;
    char min_tx_str[32];
    char mtu_str[32];

    min_tx_str[0] = 0;
    mtu_str[0] = 0;
    if (param->min_tx != 0) {
        snprintf(min_tx_str, 32, ",min_tx_size=%u", param->min_tx);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    return snprintf(desc, len, "device,%s%s%s", param->path, min_tx_str, mtu_str);
}

int pirate_device_open(void *_param, void *_ctx, int *server_fdp) {
    (void) server_fdp;
    pirate_device_param_t *param = (pirate_device_param_t *)_param;
    device_ctx *ctx = (device_ctx *)_ctx;
    pirate_device_init_param(param);
    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    if ((ctx->fd = open(param->path, ctx->flags)) < 0) {
        return -1;
    }

    if ((ctx->min_tx_buf = calloc(param->min_tx, 1)) == NULL) {
        return -1;
    }

    return 0;
}

int pirate_device_close(void *_ctx) {
    device_ctx *ctx = (device_ctx *)_ctx;
    int rv = -1;

    if (ctx->min_tx_buf != NULL) {
        free(ctx->min_tx_buf);
        ctx->min_tx_buf = NULL;
    }

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}


ssize_t pirate_device_read(const void *_param, void *_ctx, void *buf, size_t count) {
    const pirate_device_param_t *param = (const pirate_device_param_t *)_param;
    return pirate_stream_read((common_ctx*) _ctx, param->min_tx, buf, count);
}

ssize_t pirate_device_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_device_param_t *param = (const pirate_device_param_t *)_param;
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

ssize_t pirate_device_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_device_param_t *param = (const pirate_device_param_t *)_param;
    ssize_t mtu = pirate_device_write_mtu(param, _ctx);
    return pirate_stream_write((common_ctx*)_ctx, param->min_tx, mtu, buf, count);
}
