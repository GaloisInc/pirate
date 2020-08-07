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

#define ARRAY_SZ(a) (sizeof(a)/sizeof(a[0]))

typedef enum {
    MERCURY_CFG_OFF_SOURCE_ID      = 0,
    MERCURY_CFG_OFF_LEVEL          = 4,
    MERCURY_CFG_OFF_DESTINATION_ID = 8,
    MERCURY_CFG_OFF_MESSAGES       = 12
} mercury_wr_config_offset_e;

typedef enum {
    MERCURY_CFG_OFF_SESSION_ID     = 0
} mercury_rd_config_offset_e;

#define PIRATE_MERCURY_DEFAULT_FMT  "/dev/gaps_ilip_%d_%s"
#define PIRATE_MERCURY_SESSION_FMT  "/dev/gaps_ilip_s_%08x_%s"

#pragma pack(4)
typedef struct {
    uint32_t session;
    uint32_t message;
    uint32_t count;
    uint32_t data_tag;
} ilip_header_t;

typedef struct {
    uint64_t ilip_time;
    uint64_t linux_time;
} ilip_time_t;

typedef struct ilip_message {
    ilip_header_t header;
    ilip_time_t time;
    uint32_t data_length;
} ilip_message_t;
#pragma pack()

static ssize_t mercury_message_pack(void *buf, const void *data,
        uint32_t data_len, const pirate_mercury_param_t *param) {
    ilip_message_t *msg_hdr = (ilip_message_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ilip_message_t);
    const size_t msg_len = data_len + sizeof(ilip_message_t);
    struct timespec tv;
    uint64_t linux_time;

    if (msg_len > param->mtu) {
        errno = EMSGSIZE;
        return -1;
    }

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0 ) {
        return -1;
    }
    linux_time = tv.tv_sec * 1000000000ul + tv.tv_nsec;

    // Count is always 1 for now
    msg_hdr->header.count = htobe32(1u);

    // Session
    msg_hdr->header.session = htobe32(param->session.id);

    switch (param->session.id) {

        case 1:
        case 0xECA51756:
        {
            const uint32_t msg[] = {1, 2, 3};
            const uint32_t msg_tag = msg[linux_time % ARRAY_SZ(msg)];
            msg_hdr->header.message = htobe32(msg_tag);
            switch (msg_tag) {
                case 1:   msg_hdr->header.data_tag = htobe32(1u);   break;
                case 2:   msg_hdr->header.data_tag = htobe32(3u);   break;
                case 3:   msg_hdr->header.data_tag = htobe32(3u);   break;
                default:  return -1;
            }
            break;
        }

        case 2:
        case 0x67FF90F4:
        {
            const uint32_t msg[] = {2, 2, 3};
            const uint32_t msg_tag = msg[linux_time % ARRAY_SZ(msg)];
            msg_hdr->header.message = htobe32(msg_tag);
            switch (msg_tag) {
                case 2:   msg_hdr->header.data_tag = htobe32(2u);   break;
                case 3:   msg_hdr->header.data_tag = htobe32(3u);   break;
                default:  return -1;
            }
            break;
        }

        case 0x6BB83E13:
        {
            const uint32_t msg[] = {1, 5, 6};
            const uint32_t msg_tag = msg[linux_time % ARRAY_SZ(msg)];
            msg_hdr->header.message = htobe32(msg_tag);
            switch (msg_tag) {
                case 1:   msg_hdr->header.data_tag = htobe32(1u);   break;
                case 5:   msg_hdr->header.data_tag = htobe32(3u);   break;
                case 6:   msg_hdr->header.data_tag = htobe32(4u);   break;
                default:  return -1;
            }
            break;
        }

        case 0x8127AA5B:
        {
            const uint32_t msg[] = {2, 3, 4};
            const uint32_t msg_tag = msg[linux_time % ARRAY_SZ(msg)];
            msg_hdr->header.message = htobe32(msg_tag);
            switch (msg_tag) {
                case 2:   msg_hdr->header.data_tag = htobe32(1u);   break;
                case 3:   msg_hdr->header.data_tag = htobe32(1u);   break;
                case 4:   msg_hdr->header.data_tag = htobe32(2u);   break;
                default:  return -1;
            }
            break;
        }

        case 0x2C2B8E86:
        {
            msg_hdr->header.message = htobe32(1u);
            const uint32_t data[] = {1, 3, 4};
            const uint32_t data_tag = data[linux_time % ARRAY_SZ(data)];
            msg_hdr->header.data_tag = htobe32(data_tag);
            break;
        }

        case 0x442D2490:
        {
            msg_hdr->header.message = htobe32(2u);
            const uint32_t data[] = {1, 2};
            const uint32_t data_tag = data[linux_time % ARRAY_SZ(data)];
            msg_hdr->header.data_tag = htobe32(data_tag);
            break;
        }

        case 0xBC5A32FB:
        {
            msg_hdr->header.message = htobe32(1u);
            const uint32_t data[] = {2, 5};
            const uint32_t data_tag = data[linux_time % ARRAY_SZ(data)];
            msg_hdr->header.data_tag = htobe32(data_tag);
            break;
        }

        case 0x574C9A21:
        {
            msg_hdr->header.message = htobe32(2u);
            const uint32_t data[] = {1, 3, 4};
            const uint32_t data_tag = data[linux_time % ARRAY_SZ(data)];
            msg_hdr->header.data_tag = htobe32(data_tag);
            break;
        }

        default:
            return -1;
    }

    msg_hdr->time.ilip_time  = 0ul;
    msg_hdr->time.linux_time = tv.tv_sec * 1000000000ul + tv.tv_nsec;
    
    msg_hdr->data_length     = htobe32(data_len);
    memcpy(msg_data, data, data_len);
    return ((ssize_t) msg_len);
}


static ssize_t mercury_message_unpack(const void *buf, size_t buf_len,
                                        void *data, ssize_t count,
                                        const pirate_mercury_param_t *param) {
    const ilip_message_t *msg_hdr = (const ilip_message_t *)buf;
    const uint8_t *msg_data = (const uint8_t *)buf + sizeof(ilip_message_t);
    ssize_t payload_len;

    if ((be32toh(msg_hdr->header.session) != param->session.id) ||
        (be32toh(msg_hdr->header.count) != 1u)) {
        return -1;
    }

    payload_len = be32toh(msg_hdr->data_length);
    if ((payload_len + sizeof(ilip_message_t)) > buf_len) {
        errno = EIO;
        return -1;
    }

    count = MIN(payload_len, count);
    memcpy(data, msg_data, count);
    return count;
}

static void pirate_mercury_init_param(pirate_mercury_param_t *param) {
    if (param->session.level == 0) {
        param->session.level = 1;
    }

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
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.level = strtol(ptr, NULL, 10);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.source_id = strtol(ptr, NULL, 10);

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.destination_id = strtol(ptr, NULL, 10);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        key = strtok_r(ptr, KV_DELIM, &saveptr2);
        if (key == NULL) {
            errno = EINVAL;
            return -1;
        }
        val = strtok_r(NULL, KV_DELIM, &saveptr2);
        if (val == NULL) {
            if (param->session.message_count < PIRATE_MERCURY_MESSAGE_TABLE_LEN) {
                param->session.messages[param->session.message_count++] =
                    strtol(ptr, NULL, 10);
            }
        } else if (pirate_parse_is_common_key(key)) {
            continue;
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
    char *wr = desc;
    int wr_sz = 0;
    int ret_sz = 0;

    char mtu_str[32];

    mtu_str[0] = 0;
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }

    wr_sz = snprintf(wr, len, "mercury,%u,%u,%u%s",
                        param->session.level,
                        param->session.source_id,
                        param->session.destination_id,
                        mtu_str);
    ret_sz += wr_sz;

    for (uint32_t i = 0; i < param->session.message_count; ++i) {
        wr += wr_sz;
        len = MAX(len - wr_sz, 0);
        wr_sz = snprintf(wr, len, ",%u", param->session.messages[i]);
        ret_sz += wr_sz;
    }

    return ret_sz;
}

int pirate_mercury_open(void *_param, void *_ctx, int *server_fdp) {
    (void) server_fdp;
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

    if (param->session.message_count > 0) {
        const uint32_t msg_cfg_len = param->session.message_count * cfg_len;

        // Level
        sz = pwrite(fd_root, &param->session.level, cfg_len, 
                    MERCURY_CFG_OFF_LEVEL);
        if (sz != cfg_len) {
            goto error;
        }

        // Destination ID
        sz = pwrite(fd_root, &param->session.destination_id, cfg_len,
                    MERCURY_CFG_OFF_DESTINATION_ID);
        if (sz != cfg_len) {
            goto error;
        }

        // Message tags
        sz = pwrite(fd_root, param->session.messages, msg_cfg_len,
                    MERCURY_CFG_OFF_MESSAGES);
        if ((sz < 0) || (((size_t) sz) != msg_cfg_len)) {
            goto error;
        }

        sz = pwrite(fd_root, &param->session.source_id, cfg_len,
                    MERCURY_CFG_OFF_SOURCE_ID);
        if (sz != cfg_len) {
            goto error;
        }
    } else {
        sz = pwrite(fd_root, &param->session.level, cfg_len,
                    MERCURY_CFG_OFF_SOURCE_ID);
        if (sz != cfg_len) {
            goto error;
        }
    }

    sz = pread(fd_root, &param->session.id, cfg_len, MERCURY_CFG_OFF_SESSION_ID);
    if (sz != sizeof(param->session.id)) {
        goto error;
    }

    /* Must close the root device as only one open() at a time is allowed */
    if (close(fd_root) != 0) {
        goto error;
    }
    fd_root = -1;

    if (param->session.message_count == 0) {
        if (param->session.level != param->session.id) {
            errno = EBADE;
            goto error;
        }

        snprintf(ctx->path, PIRATE_LEN_NAME - 1, PIRATE_MERCURY_DEFAULT_FMT,
                    param->session.id, access == O_RDONLY ? "read" : "write");
    } else {
        snprintf(ctx->path, PIRATE_LEN_NAME - 1, PIRATE_MERCURY_SESSION_FMT,
                    param->session.id, access == O_RDONLY ? "read" : "write");
    }

    /* Open the device */
    if ((ctx->fd = open(ctx->path, ctx->flags, mode)) < 0) {
        goto error;
    }

    /* Allocate buffer for formatted messages */
    ctx->buf = (uint8_t *) malloc(param->mtu);
    if (ctx->buf == NULL) {
        goto error;
    }

    return 0;
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
    ssize_t rd_len;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    rd_len = read(ctx->fd, ctx->buf, param->mtu);
    if (rd_len < 0) {
        return -1;
    }

    return mercury_message_unpack(ctx->buf, (size_t) rd_len, buf, count, param);
}

ssize_t pirate_mercury_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        mtu = PIRATE_MERCURY_DEFAULT_MTU;
    }
    if (mtu < sizeof(ilip_message_t)) {
        errno = EINVAL;
        return -1;
    }
    return mtu - sizeof(ilip_message_t);
}

ssize_t pirate_mercury_write(const void *_param, void *_ctx, const void *buf, size_t count) {
    const pirate_mercury_param_t *param = (const pirate_mercury_param_t *)_param;
    mercury_ctx *ctx = (mercury_ctx *)_ctx;
    ssize_t wr_len, rv;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if ((wr_len = mercury_message_pack(ctx->buf, buf, count, param)) < 0) {
        return -1;
    }

    rv = write(ctx->fd, ctx->buf, wr_len);
    if (rv == wr_len) {
        return count;
    }

    return -1;
}
