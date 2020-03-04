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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/types.h>
#include "device.h"
#include "pirate_common.h"

int pirate_device_init_param(int gd, int flags, pirate_device_param_t *param) {
    (void) gd, (void) flags;
    memset(param->path, '\0', sizeof(param->path));
    param->iov_len = 0;
    return 0;
}

int pirate_device_parse_param(int gd, int flags, char *str,
                                pirate_device_param_t *param) {
    char *ptr = NULL;

    if (pirate_device_init_param(gd, flags, param) != 0) {
        return -1;
    }

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "device") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->path, ptr, sizeof(param->path));

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->iov_len = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_device_set_param(pirate_device_ctx_t *ctx,
                            const pirate_device_param_t *param) {
    if (param == NULL) {
        memset(&ctx->param, '\0', sizeof(ctx->param));
    } else {
        ctx->param = *param;
    }
    
    return 0;
}

int pirate_device_get_param(const pirate_device_ctx_t *ctx,
                            pirate_device_param_t *param) {
    *param  = ctx->param;
    return 0;
}

int pirate_device_open(int gd, int flags, pirate_device_ctx_t *ctx) {
    if (ctx->fd > 0) {
        errno = EBUSY;
        return -1;
    }

    if ((ctx->fd = open(ctx->param.path, flags)) < 0) {
        return -1;
    }
    
    return gd;
}

int pirate_device_close(pirate_device_ctx_t *ctx) {
    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}


ssize_t pirate_device_read(pirate_device_ctx_t *ctx, void *buf, size_t count) {
    return pirate_fd_read(ctx->fd, buf, count, ctx->param.iov_len);
}

ssize_t pirate_device_write(pirate_device_ctx_t *ctx, const void *buf, 
                            size_t count) {
    return pirate_fd_write(ctx->fd, buf, count, ctx->param.iov_len);
}