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

#ifndef __PIRATE_CHANNEL_UIO_INTERFACE_H
#define __PIRATE_CHANNEL_UIO_INTERFACE_H

#include "libpirate.h"
#include "shmem_buffer.h"

typedef struct {
    int flags;
    int fd;
    shmem_buffer_t *buf;
} uio_ctx;

#ifdef PIRATE_SHMEM_FEATURE

int pirate_internal_uio_parse_param(char *str, void *_param);
int pirate_internal_uio_get_channel_description(const void *_param, char *desc, int len);
int pirate_internal_uio_open(void *_param, void *_ctx);
int pirate_internal_uio_close(void *_ctx);
ssize_t pirate_internal_uio_read(const void *_param, void *_ctx, void *buf, size_t count);
ssize_t pirate_internal_uio_write(const void *_param, void *_ctx, const void *buf, size_t count);
ssize_t pirate_internal_uio_write_mtu(const void *_param);

#define PIRATE_UIO_CHANNEL_FUNCS { pirate_internal_uio_parse_param, pirate_internal_uio_get_channel_description, pirate_internal_uio_open, pirate_internal_uio_close, pirate_internal_uio_read, pirate_internal_uio_write, pirate_internal_uio_write_mtu }

#else

#define PIRATE_UIO_CHANNEL_FUNCS { NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#endif

#endif /* __PIRATE_CHANNEL_UIO_INTERFACE_H */
