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
 * Copyright 2019-2020 Two Six Labs, LLC.  All rights reserved.
 */


#include <fcntl.h>

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

#define pirate_atomic_int LONG volatile
#define ATOMIC_INC(PTR) InterlockedExchangeAdd(PTR, 1)

#include "libpirate.h"
#include "udp_socket.h"
#include "ge_eth.h"
#include "pirate_common.h"
#include "channel_funcs.h"

typedef union {
    common_ctx         common;
    udp_socket_ctx     udp_socket;
    ge_eth_ctx         ge_eth;
} pirate_channel_ctx_t;

typedef struct {
    pirate_channel_param_t param;
    pirate_channel_ctx_t ctx;
} pirate_channel_t;

static pirate_channel_t gaps_channels[PIRATE_NUM_CHANNELS];

static const pirate_channel_funcs_t gaps_channel_funcs[PIRATE_CHANNEL_TYPE_COUNT] = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    PIRATE_UDP_SOCKET_CHANNEL_FUNCS,
    PIRATE_GE_ETH_CHANNEL_FUNCS
};

int pirate_close_channel(pirate_channel_t *channel);

static inline pirate_channel_t *pirate_get_channel(int gd) {
    pirate_channel_t *channel;
    if ((gd < 0) || (gd >= PIRATE_NUM_CHANNELS)) {
        SetLastError(WSAEBADF);
        return NULL;
    }

    channel = &gaps_channels[gd];
    if (channel->param.channel_type == INVALID) {
        SetLastError(WSAEBADF);
        return NULL;
    }

    return channel;
}

static inline int pirate_channel_type_valid(channel_enum_t t) {
    if ((t <= INVALID) || (t >= PIRATE_CHANNEL_TYPE_COUNT)) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    return 0;
}

int pirate_enclave_cmpfunc(const void *a, const void *b) {
    char *s1 = *(char**) a;
    char *s2 = *(char**) b;
    if ((s1[0] == 0) && (s2[0] == 0)) {
        return 0;
    } else if (s1[0] == 0) {
        return 1;
    } else if (s2[0] == 0) {
        return -1;
    } else {
        return strncmp(s1, s2, PIRATE_LEN_NAME);
    }
}

void pirate_init_channel_param(channel_enum_t channel_type, pirate_channel_param_t *param) {
    memset(param, 0, sizeof(*param));
    param->channel_type = channel_type;
}

static const char* pirate_common_keys[] = {"src", "dst", "listener", "control", "drop", NULL};

int pirate_parse_is_common_key(const char *key) {
    for (int i = 0; pirate_common_keys[i] != NULL; i++) {
        if (strncmp(pirate_common_keys[i], key, strlen(pirate_common_keys[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

int pirate_parse_channel_param(const char *str, pirate_channel_param_t *param) {
    // Channel configuration function is allowed to modify the string
    // while braking it into delimiter-separated tokens
    char opt[256];
    strncpy_s(opt, sizeof(opt) - 1, str, _TRUNCATE);
    pirate_parse_param_t parse_func;

    pirate_init_channel_param(INVALID, param);

    if (strncmp("udp_socket", opt, strlen("udp_socket")) == 0) {
        param->channel_type = UDP_SOCKET;
    } else if (strncmp("ge_eth", opt, strlen("ge_eth")) == 0) {
        param->channel_type = GE_ETH;
    }

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    parse_func = gaps_channel_funcs[param->channel_type].parse_param;

    if (parse_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    return parse_func(opt, &param->channel);
}

int pirate_get_channel_param(int gd, pirate_channel_param_t *param) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }
    memcpy(param, &channel->param, sizeof(pirate_channel_param_t));
    return 0;
}

pirate_channel_param_t *pirate_get_channel_param_ref(int gd) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return NULL;
    }

    return &channel->param;
}

common_ctx *pirate_get_common_ctx_ref(int gd) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return NULL;
    }

    return &channel->ctx.common;
}

int pirate_unparse_channel_param(const pirate_channel_param_t *param, char *desc, int len) {
    pirate_get_channel_description_t unparse_func;
    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    unparse_func = gaps_channel_funcs[param->channel_type].get_channel_description;

    if (unparse_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    return unparse_func(&param->channel, desc, len);
}

int pirate_get_channel_description(int gd, char *desc, int len) {
    pirate_channel_t *channel = NULL;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    return pirate_unparse_channel_param(&channel->param, desc, len);
}

static pirate_atomic_int next_gd;

static int pirate_next_gd() {
    int next = ATOMIC_INC(&next_gd);
    if (next >= PIRATE_NUM_CHANNELS) {
        return -1;
    }
    return next;
}

// Declared in libpirate_internal.h for testing purposes only
void pirate_reset_gd() {
    next_gd = 0;
}

static int pirate_open(pirate_channel_t *channel) {
    pirate_channel_param_t *param = &channel->param;
    pirate_channel_ctx_t *ctx = &channel->ctx;
    int access = channel->ctx.common.flags & O_ACCMODE;
    pirate_open_t open_func;

    if ((access != O_RDONLY) && (access != O_WRONLY)) {
        SetLastError(WSAEINVAL);
        return -1;
    }

    if (pirate_write_mtu(param) < 0) {
        return -1;
    }

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    open_func = gaps_channel_funcs[param->channel_type].open;
    if (open_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    return open_func(&param->channel, ctx);
}

// gaps descriptors must be opened from smallest to largest
int pirate_open_param(pirate_channel_param_t *param, int flags) {
    pirate_channel_t channel;
    int access = flags & O_ACCMODE;

    if (next_gd >= PIRATE_NUM_CHANNELS) {
        SetLastError(WSAEMFILE);
        return -1;
    }

    memcpy(&channel.param, param, sizeof(pirate_channel_param_t));
    channel.ctx.common.flags = flags;

    if (pirate_open(&channel) < 0) {
        return -1;
    }

    int gd = pirate_next_gd();
    if (gd < 0) {
        pirate_close_channel(&channel);
        SetLastError(WSAEMFILE);
        return -1;
    }

    memcpy(&gaps_channels[gd], &channel, sizeof(pirate_channel_t));
    return gd;
}

int pirate_open_parse(const char *param, int flags) {
    pirate_channel_param_t vals;

    if (pirate_parse_channel_param(param, &vals) < 0) {
        return -1;
    }

    return pirate_open_param(&vals, flags);
}

int pirate_pipe_channel_type(channel_enum_t channel_type) {
    return 0;
}

int pirate_nonblock_channel_type(channel_enum_t channel_type) {
    switch (channel_type) {
    case UDP_SOCKET:
    case GE_ETH:
        return 1;
    default:
        return 0;
    }
}

int pirate_pipe_param(int gd[2], pirate_channel_param_t *param, int flags) {
    SetLastError(WSAEOPNOTSUPP);
    return -1;
}

int pirate_pipe_parse(int gd[2], const char *param, int flags) {
    pirate_channel_param_t vals;

    if (pirate_parse_channel_param(param, &vals) < 0) {
        return -1;
    }

    return pirate_pipe_param(gd, &vals, flags);
}

int pirate_close(int gd) {
    pirate_channel_t *channel;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }
    return pirate_close_channel(channel);
}

int pirate_close_channel(pirate_channel_t *channel) {
    int rv;
    pirate_close_t close_func;

    if (pirate_channel_type_valid(channel->param.channel_type) != 0) {
        return -1;
    }

    close_func = gaps_channel_funcs[channel->param.channel_type].close;

    if (close_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    rv = close_func(&channel->ctx);
    if (rv < 0) {
        return rv;
    }
    channel->param.channel_type = INVALID;
    return rv;
}

SSIZE_T pirate_read(int gd, void *buf, size_t count) {
    pirate_channel_t *channel = NULL;
    SSIZE_T rv;
    pirate_read_t read_func;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.common.flags & O_ACCMODE) != O_RDONLY) {
        SetLastError(WSAEBADF);
        return -1;
    }

    pirate_channel_param_t *param = &channel->param;

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    read_func = gaps_channel_funcs[param->channel_type].read;

    if (read_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    rv = read_func(&param->channel, &channel->ctx, buf, count);
    return rv;
}

SSIZE_T pirate_write(int gd, const void *buf, size_t count) {
    pirate_channel_t *channel = NULL;
    SSIZE_T rv;
    pirate_write_t write_func;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.common.flags & O_ACCMODE) != O_WRONLY) {
        SetLastError(WSAEBADF);
        return -1;
    }

    pirate_channel_param_t *param = &channel->param;

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    write_func = gaps_channel_funcs[param->channel_type].write;
    if (write_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    rv = write_func(&param->channel, &channel->ctx, buf, count);

    return rv;
}

SSIZE_T pirate_write_mtu(const pirate_channel_param_t *param) {
    pirate_write_mtu_t write_mtu_func;
    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    write_mtu_func = gaps_channel_funcs[param->channel_type].write_mtu;

    if (write_mtu_func == NULL) {
        SetLastError(WSAESOCKTNOSUPPORT);
        return -1;
    }

    return write_mtu_func(&param->channel);
}
