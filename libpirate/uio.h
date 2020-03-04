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

#ifndef __PIRATE_CHANNEL_UIO_H
#define __PIRATE_CHANNEL_UIO_H

#include "primitives.h"
#include "shmem_buffer.h"

typedef struct {
    int fd;
    shmem_buffer_t *buf;
    int flags;
    pirate_uio_param_t param;
} pirate_uio_ctx_t;

int pirate_uio_init_param(int gd, int flags, pirate_uio_param_t *param);
int pirate_uio_parse_param(int gd, int flags, char *str,
                            pirate_uio_param_t *param);
int pirate_uio_set_param(pirate_uio_ctx_t *ctx,
                            const pirate_uio_param_t *param);
int pirate_uio_get_param(const pirate_uio_ctx_t *ctx,
                            pirate_uio_param_t *param);
int pirate_uio_open(int gd, int flags, pirate_uio_ctx_t *ctx);
int pirate_uio_close(pirate_uio_ctx_t *ctx);
ssize_t pirate_uio_read(pirate_uio_ctx_t *ctx, void *buf, size_t count);
ssize_t pirate_uio_write(pirate_uio_ctx_t *ctx, const void *buf,
                            size_t count);

#endif /* __PIRATE_CHANNEL_UIO_H */
