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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pirate_common.h"
#include "pipe.h"

int pirate_pipe_init_param(int gd, int flags, pirate_pipe_param_t *param) {
    (void) flags;
    snprintf(param->dev.path, PIRATE_PIPE_NAME_LEN, PIRATE_PIPE_NAME, gd);
    param->dev.iov_len = 0;
    return 0;
}

int pirate_pipe_parse_param(char *str, pirate_pipe_param_t *param) {
    char *ptr = NULL;

    if (pirate_pipe_init_param(0, 0, param) != 0) {
        return -1;
    }

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "pipe") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->dev.path, ptr, sizeof(param->dev.path));

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->dev.iov_len = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_pipe_set_param(pirate_pipe_ctx_t *ctx, 
                            const pirate_pipe_param_t *param) {
    return pirate_device_set_param(&ctx->dev, &param->dev);
}

int pirate_pipe_get_param(const pirate_pipe_ctx_t *ctx,
                            pirate_pipe_param_t *param) {
    return pirate_device_get_param(&ctx->dev, &param->dev);
}

int pirate_pipe_open(int gd, int flags, pirate_pipe_ctx_t *ctx) {
    if (mkfifo(ctx->dev.param.path, 0660) == -1) {
        if (errno == EEXIST) {
            errno = 0;
        } else {
            return -1;
        }
    }

    return pirate_device_open(gd, flags, &ctx->dev);
}

int pirate_pipe_close(pirate_pipe_ctx_t *ctx) {
    return pirate_device_close(&ctx->dev);
}

ssize_t pirate_pipe_read(pirate_pipe_ctx_t *ctx, void *buf, size_t count) {
    return pirate_device_read(&ctx->dev, buf, count);
}

ssize_t pirate_pipe_write(pirate_pipe_ctx_t *ctx, const void *buf, 
                            size_t count) {
    return pirate_device_write(&ctx->dev, buf, count);
}
