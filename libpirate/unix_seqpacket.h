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

#ifndef __PIRATE_CHANNEL_UNIX_SEQPACKET_H
#define __PIRATE_CHANNEL_UNIX_SEQPACKET_H

#include "libpirate.h"

typedef struct {
    int flags;
    int sock;
    uint8_t *min_tx_buf;
} unix_seqpacket_ctx;

int pirate_unix_seqpacket_parse_param(char *str, void *_param);
int pirate_unix_seqpacket_get_channel_description(const void *_param, char *desc, int len);
int pirate_unix_seqpacket_open(void *_param, void *_ctx);
int pirate_unix_seqpacket_close(void *_ctx);
ssize_t pirate_unix_seqpacket_read(const void *_param, void *_ctx, void *buf, size_t count);
ssize_t pirate_unix_seqpacket_write(const void *_param, void *_ctx, const void *buf, size_t count);
ssize_t pirate_unix_seqpacket_write_mtu(const void *_param, void *_ctx);

#define PIRATE_UNIX_SEQPACKET_CHANNEL_FUNCS { pirate_unix_seqpacket_parse_param, pirate_unix_seqpacket_get_channel_description, pirate_unix_seqpacket_open, pirate_unix_seqpacket_close, pirate_unix_seqpacket_read, pirate_unix_seqpacket_write, pirate_unix_seqpacket_write_mtu }


#endif /* __PIRATE_CHANNEL_UNIX_SEQPACKET_H */
