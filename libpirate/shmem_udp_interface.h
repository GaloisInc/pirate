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

#ifndef __SHMEM_UDP_INTERFACE_H
#define __SHMEM_UDP_INTERFACE_H

#include "primitives.h"

int pirate_shmem_udp_open(int gd, int flags, pirate_channel_t *channels);
int pirate_shmem_udp_close(int gd, pirate_channel_t *channels);
ssize_t pirate_shmem_udp_read(shmem_buffer_t *shmem_buffer, void *buf,
                              size_t count);
ssize_t pirate_shmem_udp_write(shmem_buffer_t *shmem_buffer, const void *buf,
                               size_t count);
#endif
