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
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#ifndef __PIRATE_CHANNEL_SHMEM_H
#define __PIRATE_CHANNEL_SHMEM_H

#include "shmem_interface.h"

int shmem_buffer_init_param(int gd, int flags, pirate_shmem_param_t *param);
int shmem_buffer_parse_param(int gd, int flags, char *str, 
                                pirate_shmem_param_t *param);
int shmem_buffer_open(int gd, int flags, pirate_shmem_ctx_t *ctx);
int shmem_buffer_close(pirate_shmem_ctx_t *ctx);
ssize_t shmem_buffer_read(pirate_shmem_ctx_t *ctx, void *buf, size_t count);
ssize_t shmem_buffer_write(pirate_shmem_ctx_t *ctx, const void *buf, 
                            size_t count);

#endif /* __PIRATE_CHANNEL_SHMEM_H */
