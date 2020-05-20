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

#ifndef __PIRATE_COMMON_H
#define __PIRATE_COMMON_H

#include "libpirate.h"

#include <sys/types.h>

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t count;
} pirate_header_t;

typedef struct {
    int flags;
    // exists for file descriptor channel types
    int fd;
    // exists for stream-based file descriptor channel types
    uint8_t *min_tx_buf;
} common_ctx;

pirate_channel_param_t *pirate_get_channel_param_ref(int gd);
common_ctx *pirate_get_common_ctx_ref(int gd);

int pirate_enclave_cmpfunc(const void *a, const void *b);

ssize_t pirate_stream_read(common_ctx *ctx, size_t min_tx, void *buf, size_t count);
ssize_t pirate_stream_write(common_ctx *ctx, size_t min_tx, size_t write_mtu, const void *buf, size_t count);
int pirate_parse_is_common_key(const char *key);
int pirate_parse_key_value(char **key, char **val, char *ptr, char **saveptr);

#ifdef __cplusplus
}
#endif

#endif /* __PIRATE_COMMON_H */
