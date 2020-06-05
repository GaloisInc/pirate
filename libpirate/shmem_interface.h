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

#ifndef __PIRATE_CHANNEL_SHMEM_INTERFACE_H
#define __PIRATE_CHANNEL_SHMEM_INTERFACE_H

#include "libpirate.h"
#include "shmem_buffer.h"

typedef struct {
    int flags;
    shmem_buffer_t *buf;
} shmem_ctx;

int pirate_shmem_parse_param(char *str, pirate_shmem_param_t *param);
int pirate_shmem_get_channel_description(const pirate_shmem_param_t *param, char *desc, int len);
int pirate_shmem_open(pirate_shmem_param_t *param, shmem_ctx *ctx);
int pirate_shmem_close(shmem_ctx *ctx);
ssize_t pirate_shmem_read(const pirate_shmem_param_t *param, shmem_ctx *ctx, void *buf, size_t count);
ssize_t pirate_shmem_write(const pirate_shmem_param_t *param, shmem_ctx *ctx, const void *buf, size_t count);
ssize_t pirate_shmem_write_mtu(const pirate_shmem_param_t *param);

#endif /* __PIRATE_CHANNEL_SHMEM_INTERFACE_H */
