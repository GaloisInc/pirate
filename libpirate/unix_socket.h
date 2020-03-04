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

#ifndef __PIRATE_CHANNEL_UNIX_SOCKET_H
#define __PIRATE_CHANNEL_UNIX_SOCKET_H

#include "primitives.h"

typedef struct {
    int sock;
    pirate_unix_socket_param_t param;
} pirate_unix_socket_ctx_t;

int pirate_unix_socket_init_param(int gd, int flags,
                                    pirate_unix_socket_param_t *param);
int pirate_unix_socket_parse_param(int gd, int flags, char *str,
                                    pirate_unix_socket_param_t *param);
int pirate_unix_socket_set_param(pirate_unix_socket_ctx_t *ctx,
                                    const pirate_unix_socket_param_t *param);
int pirate_unix_socket_get_param(const pirate_unix_socket_ctx_t *ctx,
                                    pirate_unix_socket_param_t *param);
int pirate_unix_socket_open(int gd, int flags, pirate_unix_socket_ctx_t *ctx);
int pirate_unix_socket_close(pirate_unix_socket_ctx_t *ctx);
ssize_t pirate_unix_socket_read(pirate_unix_socket_ctx_t *ctx, void *buf,
                                size_t count);
ssize_t pirate_unix_socket_write(pirate_unix_socket_ctx_t *ctx, const void *buf,
                                    size_t count);

#endif /* __PIRATE_CHANNEL_UNIX_SOCKET_H */
