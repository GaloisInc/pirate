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

#include "uio_interface.h"
#include "uio.h"

void pirate_uio_init(pirate_channel_funcs_t *funcs) {
#ifdef PIRATE_SHMEM_FEATURE
    funcs->parse_param             = pirate_internal_uio_parse_param;
    funcs->get_channel_description = pirate_internal_uio_get_channel_description;
    funcs->open                    = pirate_internal_uio_open;
    funcs->close                   = pirate_internal_uio_close;
    funcs->read                    = pirate_internal_uio_read;
    funcs->write                   = pirate_internal_uio_write;
    funcs->write_mtu               = pirate_internal_uio_write_mtu;
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
