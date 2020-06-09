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

#include "udp_shmem_interface.h"
#include "udp_shmem.h"

void pirate_udp_shmem_init(pirate_channel_funcs_t *funcs) {
#ifdef PIRATE_SHMEM_FEATURE
    funcs->parse_param             = udp_shmem_buffer_parse_param;
    funcs->get_channel_description = udp_shmem_buffer_get_channel_description;
    funcs->open                    = udp_shmem_buffer_open;
    funcs->close                   = udp_shmem_buffer_close;
    funcs->read                    = udp_shmem_buffer_read;
    funcs->write                   = udp_shmem_buffer_write;
    funcs->write_mtu               = udp_shmem_buffer_write_mtu;
#else
    funcs->parse_param             = NULL;
    funcs->get_channel_description = NULL;
    funcs->open                    = NULL;
    funcs->close                   = NULL;
    funcs->read                    = NULL;
    funcs->write                   = NULL;
    funcs->write_mtu               = NULL;
#endif
}
