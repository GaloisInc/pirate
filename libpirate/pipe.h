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
#include "libpirate.h"

typedef struct {
    int fd;
 } pipe_ctx;

int pirate_pipe_parse_param(char *str, pirate_pipe_param_t *param);
int pirate_pipe_get_channel_description(const pirate_pipe_param_t *param, char *desc, int len);
int pirate_pipe_pipe(int flags, pirate_pipe_param_t *param, pipe_ctx *read_ctx, pipe_ctx *write_ctx);
int pirate_pipe_open(int flags, pirate_pipe_param_t *param, pipe_ctx *ctx);
int pirate_pipe_close(pipe_ctx *ctx);
ssize_t pirate_pipe_read(const pirate_pipe_param_t *param, pipe_ctx *ctx, void *buf, size_t count);
ssize_t pirate_pipe_write(const pirate_pipe_param_t *param, pipe_ctx *ctx, const void *buf, size_t count);

#endif /*__PIRATE_CHANNEL_PIPE_H */
