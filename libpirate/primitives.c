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
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic_int pirate_atomic_int;
#define ATOMIC_INC(PTR) std::atomic_fetch_add(PTR, 1)
#elif HAVE_STD_ATOMIC
#include <stdatomic.h>
typedef atomic_int pirate_atomic_int;
#define ATOMIC_INC(PTR) atomic_fetch_add(PTR, 1)
#else
typedef int pirate_atomic_int;
#define ATOMIC_INC(PTR) __atomic_fetch_add(PTR, 1, __ATOMIC_SEQ_CST)
#endif

#include "libpirate.h"
#include "device.h"
#include "pipe.h"
#include "unix_socket.h"
#include "tcp_socket.h"
#include "udp_socket.h"
#include "shmem_interface.h"
#include "udp_shmem_interface.h"
#include "uio_interface.h"
#include "serial.h"
#include "mercury.h"
#include "ge_eth.h"
#include "pirate_common.h"
#include "channel_funcs.h"

typedef union {
    common_ctx         common;
    device_ctx         device;
    pipe_ctx           pipe;
    unix_socket_ctx    unix_socket;
    tcp_socket_ctx     tcp_socket;
    udp_socket_ctx     udp_socket;
    shmem_ctx          shmem;
    udp_shmem_ctx      udp_shmem;
    uio_ctx            uio;
    serial_ctx         serial;
    mercury_ctx        mercury;
    ge_eth_ctx         ge_eth;
} pirate_channel_ctx_t;

typedef struct {
    pirate_channel_param_t param;
    pirate_channel_ctx_t ctx;
} pirate_channel_t;

static pirate_channel_t gaps_channels[PIRATE_NUM_CHANNELS];
static pirate_stats_t gaps_stats[PIRATE_NUM_CHANNELS];

static char gaps_enclave_names[PIRATE_NUM_ENCLAVES][PIRATE_LEN_NAME];
char* gaps_enclave_names_sorted[PIRATE_NUM_ENCLAVES];

int gaps_reader_gds[PIRATE_NUM_CHANNELS];
int gaps_reader_gds_num;

int gaps_writer_control_gds[PIRATE_NUM_ENCLAVES];

static const pirate_channel_funcs_t gaps_channel_funcs[PIRATE_CHANNEL_TYPE_COUNT] = {
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    PIRATE_DEVICE_CHANNEL_FUNCS,
    PIRATE_PIPE_CHANNEL_FUNCS,
    PIRATE_UNIX_SOCKET_CHANNEL_FUNCS,
    PIRATE_TCP_SOCKET_CHANNEL_FUNCS,
    PIRATE_UDP_SOCKET_CHANNEL_FUNCS,
    PIRATE_SHMEM_CHANNEL_FUNCS,
    PIRATE_UDP_SHMEM_CHANNEL_FUNCS,
    PIRATE_UIO_CHANNEL_FUNCS,
    PIRATE_SERIAL_CHANNEL_FUNCS,
    PIRATE_MERCURY_CHANNEL_FUNCS,
    PIRATE_GE_ETH_CHANNEL_FUNCS
};

int pirate_close_channel(pirate_channel_t *channel);

static inline pirate_channel_t *pirate_get_channel(int gd) {
    pirate_channel_t *channel;
    if ((gd < 0) || (gd >= PIRATE_NUM_CHANNELS)) {
        errno = EBADF;
        return NULL;
    }

    channel = &gaps_channels[gd];
    if (channel->param.channel_type == INVALID) {
        errno = EBADF;
        return NULL;
    }

    return channel;
}

static inline int pirate_channel_type_valid(channel_enum_t t) {
    if ((t <= INVALID) || (t >= PIRATE_CHANNEL_TYPE_COUNT)) {
        errno = EINVAL;
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

int pirate_declare_enclaves(int count, ...) {
    va_list ap;

    if (count > PIRATE_NUM_ENCLAVES) {
        errno = E2BIG;
        return -1;
    }
    for (int i = 0; i < PIRATE_NUM_ENCLAVES; i++) {
        gaps_enclave_names_sorted[i] = gaps_enclave_names[i];
    }
    va_start(ap, count);
    for (int i = 0; i < count; i++) {
        char *name = va_arg(ap, char*);
        strncpy(gaps_enclave_names[i], name, sizeof(gaps_enclave_names[i]) - 1);
    }
    va_end(ap);
    qsort(gaps_enclave_names_sorted, PIRATE_NUM_ENCLAVES, sizeof(char*), pirate_enclave_cmpfunc);
    return 0;
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

static int pirate_parse_common_kv(const char *key, const char *val, pirate_channel_param_t *param) {

    if (strncmp("listener", key, strlen("listener")) == 0) {
        param->listener = atoi(val);
    } else if (strncmp("control", key, strlen("control")) == 0) {
        param->control = atoi(val);
    } else if (strncmp("drop", key, strlen("drop")) == 0) {
        param->drop = atoi(val);
    } else if (strncmp("src", key, strlen("src")) == 0) {
        char **tgt = bsearch(&val, gaps_enclave_names_sorted, PIRATE_NUM_ENCLAVES, sizeof(char*), pirate_enclave_cmpfunc);
        if (tgt == NULL) {
            errno = EINVAL;
            return -1;
        }
        param->src_enclave = (tgt - gaps_enclave_names_sorted) + 1;
    } else if (strncmp("dst", key, strlen("dst")) == 0) {
        char **tgt = bsearch(&val, gaps_enclave_names_sorted, PIRATE_NUM_ENCLAVES, sizeof(char*), pirate_enclave_cmpfunc);
        if (tgt == NULL) {
            errno = EINVAL;
            return -1;
        }
        param->dst_enclave = (tgt - gaps_enclave_names_sorted) + 1;
    }
    return 0;
}

static int pirate_parse_common_param(char *str, pirate_channel_param_t *param) {
    char *token, *key, *val;
    char *saveptr1, *saveptr2;
    int rv;

    while ((token = strtok_r(str, OPT_DELIM, &saveptr1)) != NULL) {
        str = NULL;
        key = strtok_r(token, KV_DELIM, &saveptr2);
        if (key == NULL) {
            continue;
        }
        val = strtok_r(NULL, KV_DELIM, &saveptr2);
        if (val == NULL) {
            continue;
        }
        if (!pirate_parse_is_common_key(key)) {
            continue;
        }
        rv = pirate_parse_common_kv(key, val, param);
        if (rv < 0) {
            return rv;
        }
    }
    return 0;
}

int pirate_parse_channel_param(const char *str, pirate_channel_param_t *param) {
    // Channel configuration function is allowed to modify the string
    // while braking it into delimiter-separated tokens
    char opt[256];
    int rv;
    strncpy(opt, str, sizeof(opt) - 1);
    pirate_parse_param_t parse_func;

    pirate_init_channel_param(INVALID, param);

    rv = pirate_parse_common_param(opt, param);
    if (rv < 0) {
        return rv;
    }

    strncpy(opt, str, sizeof(opt) - 1);

    if (strncmp("device", opt, strlen("device")) == 0) {
        param->channel_type = DEVICE;
    } else if (strncmp("pipe", opt, strlen("pipe")) == 0) {
        param->channel_type = PIPE;
    } else if (strncmp("unix_socket", opt, strlen("unix_socket")) == 0) {
        param->channel_type = UNIX_SOCKET;
    } else if (strncmp("tcp_socket", opt, strlen("tcp_socket")) == 0) {
        param->channel_type = TCP_SOCKET;
    } else if (strncmp("udp_socket", opt, strlen("udp_socket")) == 0) {
        param->channel_type = UDP_SOCKET;
    } else if (strncmp("shmem", opt, strlen("shmem")) == 0) {
        param->channel_type = SHMEM;
    } else if (strncmp("udp_shmem", opt, strlen("udp_shmem")) == 0) {
        param->channel_type = UDP_SHMEM;
    } else if (strncmp("uio", opt, strlen("uio")) == 0) {
        param->channel_type = UIO_DEVICE;
    } else if (strncmp("serial", opt, strlen("serial")) == 0) {
        param->channel_type = SERIAL;
    } else if (strncmp("mercury", opt, strlen("mercury")) == 0) {
        param->channel_type = MERCURY;
    } else if (strncmp("ge_eth", opt, strlen("ge_eth")) == 0) {
        param->channel_type = GE_ETH;
    }

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    parse_func = gaps_channel_funcs[param->channel_type].parse_param;

    if (parse_func == NULL) {
        errno = ESOCKTNOSUPPORT;
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

const pirate_stats_t *pirate_get_stats(int gd) {

    if ((gd < 0) || (gd >= PIRATE_NUM_CHANNELS)) {
        errno = EBADF;
        return NULL;
    }

    return &gaps_stats[gd];
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
        errno = ESOCKTNOSUPPORT;
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

// Declared in libpirate_internal.h for testing purposes only
void pirate_reset_stats() {
    memset(gaps_stats, 0, sizeof(gaps_stats));
}

static int pirate_open(pirate_channel_t *channel) {
    pirate_channel_param_t *param = &channel->param;
    pirate_channel_ctx_t *ctx = &channel->ctx;
    int access = channel->ctx.common.flags & O_ACCMODE;
    int nonblock = channel->ctx.common.flags & O_NONBLOCK;
    pirate_open_t open_func;

    if ((access != O_RDONLY) && (access != O_WRONLY)) {
        errno = EINVAL;
        return -1;
    }

    if (pirate_write_mtu(param) < 0) {
        errno = EINVAL;
        return -1;
    }

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        errno = EINVAL;
        return -1;
    }

    if (nonblock && !pirate_nonblock_channel_type(param->channel_type)) {
        errno = EINVAL;
        return -1;
    }

    open_func = gaps_channel_funcs[param->channel_type].open;
    if (open_func == NULL) {
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    return open_func(&param->channel, ctx);
}

static void pirate_yield_setup(int gd, pirate_channel_param_t *param, int access) {
    if ((access == O_RDONLY) && (param->listener || param->control)) {
        gaps_reader_gds[gaps_reader_gds_num++] = gd;
    }
    if ((access == O_WRONLY) && param->control) {
        gaps_writer_control_gds[param->dst_enclave - 1] = gd;
    }
}

// gaps descriptors must be opened from smallest to largest
int pirate_open_param(pirate_channel_param_t *param, int flags) {
    pirate_channel_t channel;
    int access = flags & O_ACCMODE;

    if (next_gd >= PIRATE_NUM_CHANNELS) {
        errno = EMFILE;
        return -1;
    }

    if ((param->listener || param->control) &&
        ((param->src_enclave == 0) || (param->dst_enclave == 0) ||
         (param->src_enclave == param->dst_enclave))) {
        errno = EINVAL;
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
        errno = EMFILE;
        return -1;
    }

    memcpy(&gaps_channels[gd], &channel, sizeof(pirate_channel_t));
    pirate_yield_setup(gd, param, access);
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
    switch (channel_type) {
    case PIPE:
        return 1;
    default:
        return 0;
    }
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
    pirate_channel_t read_channel, write_channel;
    int rv, read_gd, write_gd;
    int access = flags & O_ACCMODE;
    int behavior = flags & ~O_ACCMODE;

    if (!pirate_pipe_channel_type(param->channel_type)) {
        errno = ENOSYS;
        return -1;
    }

    if (next_gd >= (PIRATE_NUM_CHANNELS - 1)) {
        errno = EMFILE;
        return -1;
    }

    if ((param->listener || param->control) &&
        ((param->src_enclave == 0) || (param->dst_enclave == 0) ||
         (param->src_enclave != param->dst_enclave))) {
        errno = EINVAL;
        return -1;
    }

    if (pirate_write_mtu(param) < 0) {
        return -1;
    }

    if (access != O_RDWR) {
        errno = EINVAL;
        return -1;
    }

    read_channel.ctx.common.flags = behavior | O_RDONLY;
    write_channel.ctx.common.flags = behavior | O_WRONLY;

    switch (param->channel_type) {
    case PIPE:
        rv = pirate_pipe_pipe(&param->channel.pipe,
            &read_channel.ctx.pipe,
            &write_channel.ctx.pipe);
        break;
    default:
        errno = ENOSYS;
        rv = -1;
    }

    memcpy(&read_channel.param, param, sizeof(pirate_channel_param_t));
    memcpy(&write_channel.param, param, sizeof(pirate_channel_param_t));

    if (rv < 0) {
        return rv;
    }

    read_gd = pirate_next_gd();
    write_gd = pirate_next_gd();
    if ((read_gd < 0) || (write_gd < 0)) {
        pirate_close_channel(&read_channel);
        pirate_close_channel(&write_channel);
        errno = EMFILE;
        return -1;
    }

    memcpy(&gaps_channels[read_gd], &read_channel, sizeof(pirate_channel_t));
    memcpy(&gaps_channels[write_gd], &write_channel, sizeof(pirate_channel_t));
    pirate_yield_setup(read_gd, param, O_RDONLY);
    pirate_yield_setup(write_gd, param, O_WRONLY);

    gd[0] = read_gd;
    gd[1] = write_gd;
    return 0;
}

int pirate_pipe_parse(int gd[2], const char *param, int flags) {
    pirate_channel_param_t vals;

    if (pirate_parse_channel_param(param, &vals) < 0) {
        return -1;
    }

    return pirate_pipe_param(gd, &vals, flags);
}

int pirate_get_fd(int gd) {
    pirate_channel_t *channel;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    switch (channel->param.channel_type) {
    case DEVICE:
    case PIPE:
    case UNIX_SOCKET:
    case TCP_SOCKET:
    case UDP_SOCKET:
    case SERIAL:
    case MERCURY:
    case GE_ETH:
        return channel->ctx.common.fd;
    default:
        errno = ENODEV;
        return -1;
    }
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
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    rv = close_func(&channel->ctx);
    if (rv < 0) {
        return rv;
    }
    channel->param.channel_type = INVALID;
    return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    pirate_channel_t *channel = NULL;
    ssize_t rv;
    pirate_read_t read_func;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.common.flags & O_ACCMODE) != O_RDONLY) {
        errno = EBADF;
        return -1;
    }

    pirate_stats_t *stats = &gaps_stats[gd];
    pirate_channel_param_t *param = &channel->param;

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    read_func = gaps_channel_funcs[param->channel_type].read;

    if (read_func == NULL) {
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    stats->requests += 1;

    rv = read_func(&param->channel, &channel->ctx, buf, count);
    if (rv < 0) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            stats->errs += 1;
        }
    } else {
        stats->success += 1;
        stats->bytes += rv;
    }
    return rv;
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
    pirate_channel_t *channel = NULL;
    ssize_t rv;
    pirate_write_t write_func;

    if ((channel = pirate_get_channel(gd)) == NULL) {
        return -1;
    }

    if ((channel->ctx.common.flags & O_ACCMODE) != O_WRONLY) {
        errno = EBADF;
        return -1;
    }

    pirate_stats_t *stats = &gaps_stats[gd];
    pirate_channel_param_t *param = &channel->param;

    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    write_func = gaps_channel_funcs[param->channel_type].write;
    if (write_func == NULL) {
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    if ((param->drop > 0) && ((stats->requests % param->drop) == 0)) {
        stats->requests += 1;
        stats->fuzzed += 1;
        return count;
    } else {
        stats->requests += 1;
    }

    rv = write_func(&param->channel, &channel->ctx, buf, count);

    if (rv < 0) {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
            stats->errs += 1;
        }
    } else {
        stats->success += 1;
        stats->bytes += rv;
        if (param->listener && !param->control) {
            int ret = pirate_listen();
            if (ret < 0) {
                return ret;
            }
        }
    }

    return rv;
}

ssize_t pirate_write_mtu(const pirate_channel_param_t *param) {
    pirate_write_mtu_t write_mtu_func;
    if (pirate_channel_type_valid(param->channel_type) != 0) {
        return -1;
    }

    write_mtu_func = gaps_channel_funcs[param->channel_type].write_mtu;

    if (write_mtu_func == NULL) {
        errno = ESOCKTNOSUPPORT;
        return -1;
    }

    return write_mtu_func(&param->channel);
}
