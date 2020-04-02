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

#include <errno.h>
#include <string.h>
#include "udp_shmem_interface.h"
#include "udp_shmem.h"

int pirate_udp_shmem_parse_param(char *str, pirate_udp_shmem_param_t *param) {
#if PIRATE_SHMEM_FEATURE
    return  udp_shmem_buffer_parse_param(str, param);
#else
    (void) str, (void) param;
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif
}

int pirate_udp_shmem_get_channel_description(const pirate_udp_shmem_param_t *param, char *desc, int len) {
#if PIRATE_SHMEM_FEATURE
    return  udp_shmem_buffer_get_channel_description(param, desc, len);
#else
    (void) param, (void) desc, (void) len;
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif
}

int pirate_udp_shmem_open(int flags, pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_open(flags, param, ctx);
#else
    (void) flags, (void) param, (void) ctx;
    return -1;
#endif
}

int pirate_udp_shmem_close(udp_shmem_ctx *ctx) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_close(ctx);
#else
    (void) ctx;
    return -1;
#endif
}

ssize_t pirate_udp_shmem_read(const pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx, void *buf, size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_read(param, ctx, buf, count);
#else
    (void) param, (void) ctx, (void) buf, (void) count;
    return -1;
#endif
}

ssize_t pirate_udp_shmem_write(const pirate_udp_shmem_param_t *param, udp_shmem_ctx *ctx, const void *buf, size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_write(param, ctx, buf, count);
#else
    (void) param, (void) ctx, (void) buf, (void) count;
    return -1;
#endif
}
