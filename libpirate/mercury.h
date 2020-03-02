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

#ifndef __PIRATE_CHANNEL_MERCURY_H
#define __PIRATE_CHANNEL_MERCURY_H

#include <stdint.h>

#define PIRATE_MERCURY_NAME_FMT     "/tmp/gaps.mercury.%d"
#define PIRATE_MERCURY_LEN_NAME      64
#define PIRATE_MERCURY_DEFAULT_MTU   256

typedef struct {
    char path[PIRATE_MERCURY_LEN_NAME];
    uint32_t mtu;
} pirate_mercury_param_t;

typedef struct {
    int fd;
    uint8_t *buf;
    pirate_mercury_param_t param;
} pirate_mercury_ctx_t;

int pirate_mercury_init_param(int gd, int flags,
                                pirate_mercury_param_t *param);
int pirate_mercury_parse_param(int gd, int flags, char *str,
                                pirate_mercury_param_t *param);
int pirate_mercury_set_param(pirate_mercury_ctx_t *ctx, 
                                const pirate_mercury_param_t *param);
int pirate_mercury_get_param(const pirate_mercury_ctx_t *ctx,
                                pirate_mercury_param_t *param);
int pirate_mercury_open(int gd, int flags, pirate_mercury_ctx_t *ctx);
int pirate_mercury_close(pirate_mercury_ctx_t *ctx);
ssize_t pirate_mercury_read(pirate_mercury_ctx_t *ctx, void *buf, size_t count);
ssize_t pirate_mercury_write(pirate_mercury_ctx_t *ctx, const void *buf, 
                                size_t count);


#if 0
#include "primitives.h"

#define MERCURY_MTU 256

int pirate_mercury_open(int gd, int flags, pirate_channel_t *channels);
int pirate_mercury_close(int gd, pirate_channel_t *channels);
ssize_t pirate_mercury_read(int gd, pirate_channel_t *readers, void *buf, 
                                size_t count);
ssize_t pirate_mercury_write(int gd, pirate_channel_t *writers, const void *buf,
                                size_t count);
#endif

#endif /* __PIRATE_CHANNEL_MERCURY_H */
