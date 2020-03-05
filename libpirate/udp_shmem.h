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

#ifndef __PIRATE_CHANNEL_UDP_SHMEM_H
#define __PIRATE_CHANNEL_UDP_SHMEM_H

#include "udp_shmem_interface.h"

int udp_shmem_buffer_parse_param(char *str, pirate_udp_shmem_param_t *param);
int udp_shmem_buffer_open(int gd, int flags, pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx);
int udp_shmem_buffer_close(udp_shmem_ctx *ctx);
ssize_t udp_shmem_buffer_read(pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx, void *buf,
                            size_t count);
ssize_t udp_shmem_buffer_write(pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx, const void *buf, 
                            size_t count);

#endif /* __PIRATE_CHANNEL_UDP_SHMEM_H */
