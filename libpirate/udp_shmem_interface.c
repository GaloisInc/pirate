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

int pirate_udp_shmem_init_param(int gd, int flags,
                                  pirate_udp_shmem_param_t *param) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_init_param(gd, flags, param);
#else
    (void) gd, (void) flags;
    memset(param, 0, sizeof(*param));
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif

}

int pirate_udp_shmem_parse_param(int gd, int flags, char *str,
                                    pirate_udp_shmem_param_t *param) {
#if PIRATE_SHMEM_FEATURE
    return  udp_shmem_buffer_parse_param(gd, flags, str, param);
#else
    (void) gd, (void) flags, (void) str, (void) param;
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif
}

int pirate_udp_shmem_set_param(pirate_udp_shmem_ctx_t *ctx,
                            const pirate_udp_shmem_param_t *param) {
#ifdef PIRATE_SHMEM_FEATURE
    if (param == NULL) {
        memset(&ctx->param, 0, sizeof(ctx->param));
    } else {
        ctx->param = *param;
    }
    
    return 0;
#else
    (void) ctx, (void) param;
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif
}

int pirate_udp_shmem_get_param(const pirate_udp_shmem_ctx_t *ctx,
                            pirate_udp_shmem_param_t *param) {
#ifdef PIRATE_SHMEM_FEATURE          
    *param  = ctx->param;
    return 0;
#else
    (void) ctx, (void) param;
    errno = ESOCKTNOSUPPORT;
    return -1;
#endif
}

int pirate_udp_shmem_open(int gd, int flags, pirate_udp_shmem_ctx_t *ctx) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_open(gd, flags, ctx);
#else
    (void) gd, (void) flags, (void) ctx;
    return -1;
#endif
}

int pirate_udp_shmem_close(pirate_udp_shmem_ctx_t *ctx) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_close(ctx);
#else
    (void) ctx;
    return -1;
#endif
}

ssize_t pirate_udp_shmem_read(pirate_udp_shmem_ctx_t *ctx, void *buf,
                                size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_read(ctx, buf, count);
#else
    (void) ctx, (void) buf, (void) count;
    return -1;
#endif
}

ssize_t pirate_udp_shmem_write(pirate_udp_shmem_ctx_t *ctx, const void *buf,
                            size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
    return udp_shmem_buffer_write(ctx, buf, count);
#else
    (void) ctx, (void) buf, (void) count;
    return -1;
#endif
}
