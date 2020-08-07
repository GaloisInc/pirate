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

#ifndef __PIRATE_CHANNEL_FUNCS_H
#define __PIRATE_CHANNEL_FUNCS_H

typedef int (*pirate_parse_param_t)(char *str, void *_param);
typedef int (*pirate_get_channel_description_t)(const void *_param, char *desc, int len);
typedef int (*pirate_open_t)(void *_param, void *ctx, int *server_fdp);
typedef int (*pirate_close_t)(void *_ctx);
typedef ssize_t (*pirate_read_t)(const void *_param, void *_ctx, void *buf, size_t count);
typedef ssize_t (*pirate_write_t)(const void *_param, void *_ctx, const void *buf, size_t count);
typedef ssize_t (*pirate_write_mtu_t)(const void *_param, void *_ctx);

typedef struct {
    pirate_parse_param_t parse_param;
    pirate_get_channel_description_t get_channel_description;
    pirate_open_t open;
    pirate_close_t close;
    pirate_read_t read;
    pirate_write_t write;
    pirate_write_mtu_t write_mtu;
} pirate_channel_funcs_t;

#endif // __PIRATE_CHANNEL_FUNCS_H
