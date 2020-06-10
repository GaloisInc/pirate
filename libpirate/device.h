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

#ifndef __PIRATE_CHANNEL_DEVICE_H
#define __PIRATE_CHANNEL_DEVICE_H

#include <sys/types.h>
#include "libpirate.h"
#include "channel_funcs.h"

typedef struct {
    int flags;
    int fd;
    uint8_t *min_tx_buf;
} device_ctx;

void pirate_device_init(pirate_channel_funcs_t *funcs);

#endif /*__PIRATE_CHANNEL_DEVICE_H */
