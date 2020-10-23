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

#ifdef PIRATE_SHMEM_FEATURE

int shmem_buffer_parse_param(char *str, void *_param);
int shmem_buffer_get_channel_description(const void *_param, char *desc, int len);
int shmem_buffer_open(void *_param, void *_ctx);
int shmem_buffer_close(void *_ctx);
ssize_t shmem_buffer_read(const void *_param, void *_ctx, void *buf, size_t count);
ssize_t shmem_buffer_write(const void *_param, void *_ctx, const void *buf, size_t count);
ssize_t shmem_buffer_write_mtu(const void *_param, void *_ctx);

#define PIRATE_SHMEM_CHANNEL_FUNCS { shmem_buffer_parse_param, shmem_buffer_get_channel_description, shmem_buffer_open, shmem_buffer_close, shmem_buffer_read, shmem_buffer_write, shmem_buffer_write_mtu }

#else

#define PIRATE_SHMEM_CHANNEL_FUNCS { NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#endif

#endif /* __PIRATE_CHANNEL_SHMEM_INTERFACE_H */
