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

#ifndef __PIRATE_CHANNEL_UDP_SHMEM_INTERFACE_H
#define __PIRATE_CHANNEL_UDP_SHMEM_INTERFACE_H

#include "libpirate.h"
#include "shmem_buffer.h"

typedef struct {
    int flags;
    shmem_buffer_t *buf;
} udp_shmem_ctx;

#ifdef PIRATE_SHMEM_FEATURE

int udp_shmem_buffer_parse_param(char *str, void *_param);
int udp_shmem_buffer_get_channel_description(const void *_param, char *desc, int len);
int udp_shmem_buffer_open(void *_param, void *_ctx);
int udp_shmem_buffer_close(void *_ctx);
ssize_t udp_shmem_buffer_read(const void *_param, void *_ctx, void *buf, size_t count);
ssize_t udp_shmem_buffer_write(const void *_param, void *_ctx, const void *buf,  size_t count);
ssize_t udp_shmem_buffer_write_mtu(const void *_param);

#define PIRATE_UDP_SHMEM_CHANNEL_FUNCS { udp_shmem_buffer_parse_param, udp_shmem_buffer_get_channel_description, udp_shmem_buffer_open, udp_shmem_buffer_close, udp_shmem_buffer_read, udp_shmem_buffer_write, udp_shmem_buffer_write_mtu }

#else

#define PIRATE_UDP_SHMEM_CHANNEL_FUNCS { NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#endif

#endif /* __PIRATE_CHANNEL_UDP_SHMEM_INTERFACE_H */
