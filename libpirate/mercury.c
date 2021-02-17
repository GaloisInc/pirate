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

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pirate_common.h"
#include "mercury.h"

typedef enum {
    MERCURY_CFG_OFF_SESSION_ID     = 0
} mercury_config_offset_e;

#define PIRATE_MERCURY_DEFAULT_FMT  "/dev/gaps_ilip_%d_%s"
#define PIRATE_MERCURY_SESSION_FMT  "/dev/gaps_ilip_s_%08x_%s"

#pragma pack(4)
typedef struct {
    uint32_t session;
    uint32_t message;
    uint32_t descriptor_type;
    // The following field is labelled
    // data tag for short messages and
    // descriptor tag for long messages.
    uint32_t data_or_descriptor_tag;
} ilip_header_t;

typedef struct {
    uint64_t ilip_time;
    uint64_t linux_time;
} ilip_time_t;

typedef struct ilip_message {
    ilip_header_t header;
    ilip_time_t time;
    uint32_t dest_tag;
    uint32_t immediate_length;
} ilip_message_t;

typedef struct ilip_long_message {
    ilip_message_t immediate;
    uint64_t data_hash_lo;
    uint64_t data_hash_hi;
    uint32_t data_tag;
    uint32_t data_length;
    uint64_t host_payload_address;
    uint8_t unused[176];
    uint64_t desc_hash;
} ilip_long_message_t;

#pragma pack()

static int mercury_message_pack(void *buf, const void *data,
        uint32_t data_len, const pirate_mercury_param_t *param) {
    ilip_message_t *msg_hdr = (ilip_message_t *)buf;
    struct timespec tv;
    uint64_t linux_time;

    if ((param->mode == MERCURY_IMMEDIATE) && (data_len > PIRATE_MERCURY_IMMEDIATE_SIZE)) {
        errno = EMSGSIZE;
        return -1;
    }

    if ((param->mtu > 0) && (data_len > param->mtu)) {
        errno = EMSGSIZE;
        return -1;
    }

    // Hash computations require unused fields to be populated by zeros.
    memset(buf, 0, PIRATE_MERCURY_DMA_DESCRIPTOR);

    if (param->mode == MERCURY_IMMEDIATE) {
        msg_hdr->header.descriptor_type = 0;
    } else {
        msg_hdr->header.descriptor_type = 0x10000000u;
    }

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0 ) {
        return -1;
    }
    linux_time = tv.tv_sec * 1000000000ul + tv.tv_nsec;

    // Session
    msg_hdr->header.session = htobe32(param->session_id);
    msg_hdr->header.message = htobe32(param->message_id);

    msg_hdr->time.ilip_time  = 0ul;
    msg_hdr->time.linux_time = linux_time;

    if (param->mode == MERCURY_IMMEDIATE) {
        uint8_t *immediate_data = (uint8_t *)buf + sizeof(ilip_message_t);
        msg_hdr->header.data_or_descriptor_tag = htobe32(param->data_tag);
        msg_hdr->immediate_length = htobe32(data_len);
        memcpy(immediate_data, data, data_len);
    } else {
        ilip_long_message_t *long_msg_hdr = (ilip_long_message_t *)buf;
        msg_hdr->header.data_or_descriptor_tag = htobe32(param->descriptor_tag);
        long_msg_hdr->data_tag = htobe32(param->data_tag);
        long_msg_hdr->host_payload_address = (uintptr_t) data;
        long_msg_hdr->data_length = htobe32(data_len);
        // TODO calculate message data siphash
    }

    // TODO calculate DMA descriptor siphash
    return 0;
}

static void pirate_mercury_init_param(pirate_mercury_param_t *param) {
    if (param->mtu == 0) {
        param->mtu = PIRATE_MERCURY_DEFAULT_MTU;
    }
}

int pirate_mercury_parse_param(char *str, void *_param) {
    pirate_mercury_param_t *param = (pirate_mercury_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "mercury") != 0)) {
        errno = EINVAL;
        return -1;
    }

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        key = strtok_r(ptr, KV_DELIM, &saveptr2);
        if (key == NULL) {
            errno = EINVAL;
            return -1;
        }
        val = strtok_r(NULL, KV_DELIM, &saveptr2);
        if (pirate_parse_is_common_key(key)) {
            continue;
        } else if (strncmp("mode", key, strlen("mode")) == 0) {
            if (strncmp("immediate", val, sizeof("immediate")) == 0) {
                param->mode = MERCURY_IMMEDIATE;
            } else if (strncmp("payload", val, sizeof("payload")) == 0) {
                param->mode = MERCURY_PAYLOAD;
            }
        } else if (strncmp("session", key, strlen("session")) == 0) {
            param->session_id = strtol(val, NULL, 10);
        } else if (strncmp("message", key, strlen("message")) == 0) {
            param->message_id = strtol(val, NULL, 10);
        } else if (strncmp("data", key, strlen("data")) == 0) {
            param->data_tag = strtol(val, NULL, 10);
        } else if (strncmp("descriptor", key, strlen("descriptor")) == 0) {
            param->descriptor_tag = strtol(val, NULL, 10);
        } else if (strncmp("mtu", key, strlen("mtu")) == 0) {
            param->mtu = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_mercury_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    int ret_sz = 0;
    char mode[16], mtu_str[32];

    switch (param->mode) {
        case MERCURY_IMMEDIATE:
            strncpy(mode, "immediate", sizeof(mode));
            break;
        case MERCURY_PAYLOAD:
            strncpy(mode, "payload", sizeof(mode));
            break;
        default:
            strncpy(mode, "unknown", sizeof(mode));
            break;
    }

    mtu_str[0] = 0;
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }

    ret_sz = snprintf(desc, len, "mercury,mode=%s,session=%u,message=%u,data=%u,descriptor=%u%s",
                        mode,
                        param->session_id,
                        param->message_id,
                        param->data_tag,
                        param->descriptor_tag,
                        mtu_str);
    return ret_sz;
}

int pirate_mercury_open(void *_param, void *_ctx) {
    pirate_mercury_param_t *param = (pirate_mercury_param_t *)_param;
    mercury_ctx *ctx = (mercury_ctx *)_ctx;
    const uint32_t cfg_len = sizeof(uint32_t);
    ssize_t sz;
    int fd_root = -1;
    unsigned wait_counter = 0;
    int access = ctx->flags & O_ACCMODE;
    const mode_t mode = access == O_RDONLY ? S_IRUSR : S_IWUSR;

    /* Open the root device to configure and establish a session */
    pirate_mercury_init_param(param);

    if ((param->mode != MERCURY_IMMEDIATE) && (param->mode != MERCURY_PAYLOAD)) {
        errno = EINVAL;
        return -1;
    }
    if ((param->mode == MERCURY_IMMEDIATE) &&
        (param->session_id != 1) && (param->session_id != 2)) {
        // we have dropped support for any sessions other than 1 and 2
        // in MERCURY_IMMEDIATE mode
        errno = EINVAL;
        return -1;
    }

    /* Root device enforces single open */
    for(;;) {
        int err = errno;
        fd_root = open(PIRATE_MERCURY_ROOT_DEV, O_RDWR, S_IRUSR | S_IWUSR);
        if (fd_root != -1) {
            break;
        }
    
        if (errno != EBUSY) {
            return -1;
        } else {
            usleep(100);
            /* Timeout 1 second */
            if (++wait_counter >= 10 * 1000) {
                errno = ETIME;
                return -1;
            }
            errno = err;
        }
    }

    sz = pwrite(fd_root, &param->session_id, cfg_len,
                MERCURY_CFG_OFF_SESSION_ID);
    if (sz != cfg_len) {
        goto error;
    }

    /* Must close the root device as only one open() at a time is allowed */
    if (close(fd_root) != 0) {
        goto error;
    }
    fd_root = -1;

    snprintf(ctx->path, PIRATE_LEN_NAME - 1, PIRATE_MERCURY_DEFAULT_FMT,
             param->mode, access == O_RDONLY ? "read" : "write");

    /* Open the device */
    if ((ctx->fd = open(ctx->path, ctx->flags, mode)) < 0) {
        goto error;
    }

    /* Allocate buffer for formatted messages */
    if ((param->mode == MERCURY_PAYLOAD) && (access == O_RDONLY)) {
        // this should go away when the read() API is fixed
        ctx->buf = (uint8_t *) malloc(param->mtu + PIRATE_MERCURY_DMA_DESCRIPTOR);
    } else {
        ctx->buf = (uint8_t *) malloc(PIRATE_MERCURY_DMA_DESCRIPTOR);
    }
    if (ctx->buf == NULL) {
        goto error;
    }

    return ctx->fd;
error:
    if (fd_root > 0) {
        close(fd_root);
        fd_root = -1;
    }

    if (ctx->fd > 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
    }

    return -1;
}

int pirate_mercury_close(void *_ctx) {
    mercury_ctx *ctx = (mercury_ctx *)_ctx;
    int rv = -1;

    if (ctx->buf != NULL) {
        free(ctx->buf);
        ctx->buf = NULL;
    }

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ctx->fd);
    ctx->fd = -1;
    return rv;
}

ssize_t pirate_mercury_read(const void *_param, void *_ctx, void *buf, size_t count) {
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    mercury_ctx *ctx = (mercury_ctx *)_ctx;
    ilip_message_t *msg_hdr = (ilip_message_t *) ctx->buf;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if (param->mode == MERCURY_IMMEDIATE) {
        size_t rd_len = PIRATE_MERCURY_DMA_DESCRIPTOR;
        ssize_t rv = read(ctx->fd, ctx->buf, rd_len);
        if (rv < 0) {
            return -1;
        }
        const uint8_t *msg_data = (const uint8_t *) ctx->buf + sizeof(ilip_message_t);
        uint32_t payload_len = be32toh(msg_hdr->immediate_length);
        count = MIN(payload_len, count);
        memcpy(buf, msg_data, count);
    } else {
        // this is fine (ᓄ ᴥ ᓇ)
        size_t rd_len = PIRATE_MERCURY_DMA_DESCRIPTOR + count;
        ssize_t rv = read(ctx->fd, ctx->buf, rd_len);
        if (rv < PIRATE_MERCURY_DMA_DESCRIPTOR) {
            return -1;
        }
        // The data_length field of the descriptor is not set.
        // Use the read() return value instead.
        count = MIN((size_t) (rv - PIRATE_MERCURY_DMA_DESCRIPTOR), count);
        memcpy(buf, ctx->buf + PIRATE_MERCURY_DMA_DESCRIPTOR, count);
    }
    return count;
}

ssize_t pirate_mercury_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    return param->mtu;
}

ssize_t pirate_mercury_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    mercury_ctx *ctx = (mercury_ctx *)_ctx;
    ssize_t wr_len, rv;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if (mercury_message_pack(ctx->buf, buf, count, param)) {
        return -1;
    }

    if (param->mode == MERCURY_IMMEDIATE) {
        wr_len = PIRATE_MERCURY_DMA_DESCRIPTOR;
    } else {
        // this is fine (ᓄ ᴥ ᓇ)
        wr_len = PIRATE_MERCURY_DMA_DESCRIPTOR + count;
    }

    rv = write(ctx->fd, ctx->buf, wr_len);
    if (rv == wr_len) {
        return count;
    }

    return -1;
}
