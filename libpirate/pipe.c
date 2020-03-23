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
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pirate_common.h"
#include "pipe.h"

int pirate_pipe_parse_param(char *str, pirate_pipe_param_t *param) {
    char *ptr = NULL;

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) ||
        (strcmp(ptr, "pipe") != 0)) {
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

int pirate_pipe_open(int flags, pirate_pipe_param_t *param, pipe_ctx *ctx) {
    int err;

    if (strnlen(param->path, 1) == 0) {
        errno = EINVAL;
        return -1;
    }
    err = errno;
    if (mkfifo(param->path, 0660) == -1) {
        if (errno == EEXIST) {
            errno = err;
        } else {
            return -1;
        }
    }

    if ((ctx->fd = open(param->path, flags)) < 0) {
        return -1;
    }

    return 0;
}

int pirate_pipe_pipe(int flags, pirate_pipe_param_t *param, pipe_ctx *read_ctx, pipe_ctx *write_ctx) {
    (void) flags;
    (void) param;
    int rv, fd[2];

    rv = pipe(fd);
    if (rv < 0) {
        return rv;
    }

    read_ctx->fd = fd[0];
    write_ctx->fd = fd[1];
    return 0;
}

int pirate_pipe_close(pipe_ctx *ctx) {
    int rv = -1;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_pipe_read(const pirate_pipe_param_t *param, pipe_ctx *ctx, void *buf, size_t count) {
    return pirate_fd_read(ctx->fd, buf, count, param->iov_len);
}

ssize_t pirate_pipe_write(const pirate_pipe_param_t *param, pipe_ctx *ctx, const void *buf, size_t count) {
    return pirate_fd_write(ctx->fd, buf, count, param->iov_len);
}
