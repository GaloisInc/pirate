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

#ifndef __PIRATE_CHANNEL_MERCURY_H
#define __PIRATE_CHANNEL_MERCURY_H

#include "libpirate.h"

typedef struct {
    int fd;
    int flags;
    uint8_t *buf;
    char path[PIRATE_LEN_NAME];
} mercury_ctx;

int pirate_mercury_parse_param(char *str, pirate_mercury_param_t *param);
int pirate_mercury_open(int flags, pirate_mercury_param_t *param, mercury_ctx *ctx);
int pirate_mercury_close(mercury_ctx *ctx);
ssize_t pirate_mercury_read(const pirate_mercury_param_t *param, mercury_ctx *ctx, void *buf, size_t count);
ssize_t pirate_mercury_write(const pirate_mercury_param_t *param, mercury_ctx *ctx, const void *buf, size_t count);

#endif /* __PIRATE_CHANNEL_MERCURY_H */
