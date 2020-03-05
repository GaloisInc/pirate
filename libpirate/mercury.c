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
        uint32_t data_len, const pirate_mercury_param_t *param,
        const mercury_ctx *ctx) {
    ilip_message_t *msg_hdr = (ilip_message_t *)buf;
    uint8_t *msg_data = (uint8_t *)buf + sizeof(ilip_message_t);
    const ssize_t msg_len = data_len + sizeof(ilip_message_t);
    struct timespec tv;
    (void) ctx;

    if (msg_len > param->mtu) {
        errno = ENOBUFS;
        return -1;
    }

    if(clock_gettime(CLOCK_REALTIME, &tv) != 0 ) {
        return -1;
    }

    msg_hdr->header.session  = htobe32(1u);
    msg_hdr->header.message  = htobe32(1u);
    msg_hdr->header.count    = htobe32(1u);
    msg_hdr->header.data_tag = htobe32(1u);
    msg_hdr->time.ilip_time  = 0ul;
    msg_hdr->time.linux_time = tv.tv_sec * 1000000000ul + tv.tv_nsec;
    msg_hdr->data_length     = htobe32(data_len);

    memcpy(msg_data, data, data_len);
    return msg_len;
}


static ssize_t mercury_message_unpack(const void *buf, ssize_t buf_len,
                                        void *data, ssize_t count,
                                        const pirate_mercury_param_t *param,
                                        const mercury_ctx *ctx) {
    (void) ctx;

    const ilip_message_t *msg_hdr = (const ilip_message_t *)buf;
    const uint8_t *msg_data = (const uint8_t *)buf + sizeof(ilip_message_t);
    ssize_t payload_len;

    if (buf_len > param->mtu) {
        errno = ENOBUFS;
        return -1;
    }

    if ((be32toh(msg_hdr->header.session) != 1u) ||
        (be32toh(msg_hdr->header.message) != 1u) ||
        (be32toh(msg_hdr->header.count) != 1u) ||
        (be32toh(msg_hdr->header.data_tag) != 1u)) {
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

static void pirate_mercury_init_param(int gd, int flags,
                                        pirate_mercury_param_t *param) {
    if (param->application_id == 0) {
        param->application_id = gd + 1;
    }

    if (strnlen(param->path, 1) == 0) {
        snprintf(param->path, PIRATE_LEN_NAME - 1, PIRATE_MERCURY_NAME_FMT, 
                    param->application_id,
                    flags == O_RDONLY ? "read" : "write");
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

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->application_id = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->path, ptr, sizeof(param->path));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->mtu = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->timeout_ms = strtol(ptr, NULL, 10);
    }

    return 0;
}

int pirate_mercury_open(int gd, int flags, pirate_mercury_param_t *param, 
                            mercury_ctx *ctx) {
    ssize_t sz;
    int fd_root = -1;
    ctx->flags = flags;
    const mode_t mode = ctx->flags == O_RDONLY ? S_IRUSR : S_IWUSR;

    /* Current implementation is limited to two channels */
    if (gd > 1) {
        errno = ELNRNG;
        return -1;
    }

    /* Open the root device to convert application ID to session ID */
    pirate_mercury_init_param(gd, flags, param);

    if (pthread_mutex_lock(&open_lock) != 0) {
        return -1;
    }

    fd_root = open(PIRATE_MERCURY_ROOT_DEV, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_root == -1) {
        goto error_session;
    }

    sz = write(fd_root, &param->application_id, sizeof(param->application_id));
    if (sz != sizeof(param->application_id)) {
        goto error_session;
    }

    sz = read(fd_root, &ctx->session_id, sizeof(ctx->session_id));
    if (sz != sizeof(ctx->session_id)) {
        goto error_session;
    }

    if (param->application_id != ctx->session_id) {
        errno = EBADE;
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

    /* Open the device */
    if ((ctx->fd = open(param->path, ctx->flags, mode)) < 0) {
        goto error;
    }

    /* Allocate buffer for formatted messages */
    ctx->buf = (uint8_t *) malloc(param->mtu);
    if (ctx->buf == NULL) {
        goto error;
    }

    return gd;
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

ssize_t pirate_mercury_read(pirate_mercury_param_t *param, mercury_ctx *ctx,
                            void *buf, size_t count) {
    ssize_t rd_len;
    int do_read = 1;
    uint32_t wait_counter = 0;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    do {
        rd_len = read(ctx->fd, ctx->buf, param->mtu);
        if (rd_len < 0) {
            if (errno != EAGAIN) {
                return  -1;
            } else {
                usleep(100);
                if (++wait_counter >= (10*param->timeout_ms) ) {
                    return -1;
                }
                errno = 0;
            }
        } else {
            do_read = 0;
        }
    } while(do_read == 1);

    return mercury_message_unpack(ctx->buf, rd_len, buf, count, param, ctx);
}

ssize_t pirate_mercury_write(pirate_mercury_param_t *param, mercury_ctx *ctx,
                                const void *buf, size_t count) {
    ssize_t wr_len;
    int do_write = 1;
    uint32_t wait_counter = 0;

    if (ctx->fd <= 0) {
        errno = ENODEV;
        return -1;
    }

    if ((wr_len = mercury_message_pack(ctx->buf, buf, count, param, ctx)) < 0) {
        return -1;
    }

    do {
        ssize_t rv = write(ctx->fd, ctx->buf, wr_len);
        if (rv < 0) {
            if (errno != EAGAIN) {
                return  -1;
            } else {
                usleep(100);
                if (++wait_counter >= 10 * param->timeout_ms) {
                    errno = ETIME;
                    return -1;
                }
                errno = 0;
            }
        } else if (rv == wr_len) {
            return count;
        }
    } while(do_write == 1);

    return -1;
}
