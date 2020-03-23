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
#include <pthread.h>
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

static pthread_mutex_t open_lock = PTHREAD_MUTEX_INITIALIZER;

static ssize_t mercury_message_pack(void *buf, const void *data,
        uint32_t data_len, const pirate_mercury_param_t *param) {
    ilip_message_t *msg_hdr = (ilip_message_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ilip_message_t);
    const ssize_t msg_len = data_len + sizeof(ilip_message_t);
    struct timespec tv;
    uint64_t linux_time;

    if (msg_len > param->mtu) {
        errno = ENOBUFS;
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
    return msg_len;
}


static ssize_t mercury_message_unpack(const void *buf, ssize_t buf_len,
                                        void *data, ssize_t count,
                                        const pirate_mercury_param_t *param) {
    const ilip_message_t *msg_hdr = (const ilip_message_t *)buf;
    const uint8_t *msg_data = (const uint8_t *)buf + sizeof(ilip_message_t);
    ssize_t payload_len;

    if (buf_len > param->mtu) {
        errno = ENOBUFS;
        return -1;
    }

    if ((be32toh(msg_hdr->header.session) != param->session.id) ||
        (be32toh(msg_hdr->header.count) != 1u)) {
        return -1;
    }

    payload_len = be32toh(msg_hdr->data_length);
    if ((payload_len + sizeof(ilip_message_t)) > param->mtu) {
        errno = EIO;
        return -1;
    }

    count = MIN(payload_len, count);
    memcpy(data, msg_data, count);
    return count;
}

static void pirate_mercury_init_param(pirate_mercury_param_t *param) {
    if (param->session.source_id == 0) {
        param->session.source_id = 1;
    }

    if (param->mtu == 0) {
        param->mtu = PIRATE_MERCURY_DEFAULT_MTU;
    }

    if (param->timeout_ms == 0) {
        param->timeout_ms = PIRATE_MERCURY_DEFAULT_TIMEOUT_MS;
    }
}

int pirate_mercury_parse_param(char *str, pirate_mercury_param_t *param) {
    char *ptr = NULL;

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) ||
        (strcmp(ptr, "mercury") != 0)) {
        return -1;
    }

    // Non-parsed and default parameters
    param->mtu = PIRATE_MERCURY_DEFAULT_MTU;
    param->timeout_ms = PIRATE_MERCURY_DEFAULT_TIMEOUT_MS;

    // Level
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.level = strtol(ptr, NULL, 10);

    // Source ID
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.source_id = strtol(ptr, NULL, 10);

    // Destination ID
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    param->session.destination_id = strtol(ptr, NULL, 10);

    // Timeout
    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->timeout_ms = strtol(ptr, NULL, 10);
    }

    // Messages
    while (((ptr = strtok(NULL, OPT_DELIM)) != NULL) &&
           (param->session.message_count < PIRATE_MERCURY_MESSAGE_TABLE_LEN)) {
        param->session.messages[param->session.message_count++] = 
            strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_mercury_open(int flags, pirate_mercury_param_t *param, mercury_ctx *ctx) {
    const uint32_t cfg_len = sizeof(uint32_t);
    ssize_t sz;
    int fd_root = -1;
    ctx->flags = flags;
    int access = ctx->flags & O_ACCMODE;
    const mode_t mode = access == O_RDONLY ? S_IRUSR : S_IWUSR;

    /* Open the root device to configure and establish a session */
    pirate_mercury_init_param(param);

    if (pthread_mutex_lock(&open_lock) != 0) {
        return -1;
    }

    fd_root = open(PIRATE_MERCURY_ROOT_DEV, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_root == -1) {
        goto error_session;
    }

    if (param->session.message_count > 0) {
        const uint32_t msg_cgf_len = param->session.message_count * cfg_len;

        // Level
        sz = pwrite(fd_root, &param->session.level, cfg_len, 
                    MERCURY_CFG_OFF_LEVEL);
        if (sz != cfg_len) {
            goto error_session;
        }

        // Destination ID
        sz = pwrite(fd_root, &param->session.destination_id, cfg_len,
                    MERCURY_CFG_OFF_DESTINATION_ID);
        if (sz != cfg_len) {
            goto error_session;
        }

        // Message tags
        sz = pwrite(fd_root, param->session.messages, msg_cgf_len,
                    MERCURY_CFG_OFF_MESSAGES);
        if (sz != msg_cgf_len) {
            goto error_session;
        }
    }

    sz = pwrite(fd_root, &param->session.source_id, cfg_len,
                    MERCURY_CFG_OFF_SOURCE_ID);
    if (sz != cfg_len) {
        goto error_session;
    }

    sz = pread(fd_root, &param->session.id, cfg_len, MERCURY_CFG_OFF_SESSION_ID);
    if (sz != sizeof(param->session.id)) {
        goto error_session;
    }

    /* Must close the root device as only one open() at a time is allowed */
    if (close(fd_root) != 0) {
        goto error_session;
    }
    fd_root = -1;

    if (pthread_mutex_unlock(&open_lock) != 0) {
        goto error;
    }

    if (param->session.message_count == 0) {
        if (param->session.source_id != param->session.id) {
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
error_session:
    pthread_mutex_unlock(&open_lock);
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

int pirate_mercury_close(mercury_ctx *ctx) {
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

ssize_t pirate_mercury_read(const pirate_mercury_param_t *param,
                            mercury_ctx *ctx, void *buf, size_t count) {
    ssize_t rd_len;
    int err;
    uint32_t wait_counter = 0;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    for(;;) {
        err = errno;
        rd_len = read(ctx->fd, ctx->buf, param->mtu);
        if (rd_len >= 0) {
            break;
        }
        if (errno != EAGAIN) {
            return -1;
        } else {
            usleep(100);
            if (++wait_counter >= (10*param->timeout_ms) ) {
                return -1;
            }
            errno = err;
        }
    }

    return mercury_message_unpack(ctx->buf, rd_len, buf, count, param);
}

ssize_t pirate_mercury_write(const pirate_mercury_param_t *param,
                             mercury_ctx *ctx, const void *buf, size_t count) {
    ssize_t wr_len;
    int err;
    uint32_t wait_counter = 0;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if ((wr_len = mercury_message_pack(ctx->buf, buf, count, param)) < 0) {
        return -1;
    }

    for(;;) {
        err = errno;
        ssize_t rv = write(ctx->fd, ctx->buf, wr_len);
        if (rv < 0) {
            if (errno != EAGAIN) {
                return -1;
            } else {
                usleep(100);
                if (++wait_counter >= 10 * param->timeout_ms) {
                    errno = ETIME;
                    return -1;
                }
                errno = err;
            }
        } else if (rv == wr_len) {
            return count;
        }
    }

    return -1;
}
