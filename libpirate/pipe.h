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

#ifndef __PIRATE_CHANNEL_PIPE_H
#define __PIRATE_CHANNEL_PIPE_H

#include <sys/types.h>
#include "primitives.h"

typedef struct {
    int fd;
    pirate_pipe_param_t param;
 } pirate_pipe_ctx_t;

int pirate_pipe_init_param(int gd, int flags, pirate_pipe_param_t *param);
int pirate_pipe_parse_param(int gd, int flags, char *str,
                            pirate_pipe_param_t *param);
int pirate_pipe_set_param(pirate_pipe_ctx_t *ctx,
                            const pirate_pipe_param_t *param);
int pirate_pipe_get_param(const pirate_pipe_ctx_t *ctx,
                            pirate_pipe_param_t *param);
int pirate_pipe_open(int gd, int flags, pirate_pipe_ctx_t *ctx);
int pirate_pipe_close(pirate_pipe_ctx_t *ctx);
ssize_t pirate_pipe_read(pirate_pipe_ctx_t *ctx, void *buf, size_t count);
ssize_t pirate_pipe_write(pirate_pipe_ctx_t *ctx, const void *buf,
                            size_t count);

#endif /*__PIRATE_CHANNEL_PIPE_H */
