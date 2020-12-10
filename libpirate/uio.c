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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "pirate_common.h"
#include "uio_interface.h"

// TODO: replace reader and writer with BipBuffer
// https://ferrous-systems.com/blog/lock-free-ring-buffer/
// use 20 bits for reader, 20 bits for writer, 20 bits for watermark
// use remaining 4 bits for status
// use status bits to indicate whether empty or full

static inline int buffer_size() {
    return getpagesize() * 256;
}

static inline uint8_t get_status(uint64_t value) {
    return (value & 0xff00000000000000) >> 56;
}

static inline uint32_t get_write(uint64_t value) {
    return (value & 0x00fffffff0000000) >> 28;
}

static inline uint32_t get_read(uint64_t value) {
    return (value & 0x000000000fffffff);
}

static inline uint8_t* shared_buffer(shmem_buffer_t *uio_buffer) {
    return (uint8_t *) uio_buffer + sizeof(shmem_buffer_t);
}

static inline uint64_t create_position(uint32_t write, uint32_t read,
                                       int writer) {
    uint8_t status = 1;

    if (read == write) {
        status = writer ? 2 : 0;
    }

    return (((uint64_t)status) << 56) | (((uint64_t)write) << 28) | read;
}

static inline int is_empty(uint64_t value) {
    return get_status(value) == 0;
}

static inline int is_full(uint64_t value) {
    return get_status(value) == 2;
}

static void pirate_uio_init_param(pirate_uio_param_t *param) {
    if (strnlen(param->path, 1) == 0) {
        snprintf(param->path, PIRATE_LEN_NAME - 1, PIRATE_UIO_DEFAULT_PATH);
    }
}

int pirate_internal_uio_parse_param(char *str, void *_param) {
    pirate_uio_param_t *param = (pirate_uio_param_t *)_param;
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "uio") != 0)) {
        return -1;
    }

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("path", key, strlen("path")) == 0) {
            strncpy(param->path, val, sizeof(param->path) - 1);
        } else if (strncmp("max_tx_size", key, strlen("max_tx_size")) == 0) {
            param->max_tx = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int pirate_internal_uio_get_channel_description(const void *_param, char *desc, int len) {
    const pirate_uio_param_t *param = (const pirate_uio_param_t *)_param;
    char max_tx_str[32];
    char mtu_str[32];

    max_tx_str[0] = 0;
    mtu_str[0] = 0;
    if ((param->max_tx != 0) && (param->max_tx != PIRATE_UIO_DEFAULT_MAX_TX)) {
        snprintf(max_tx_str, 32, ",max_tx_size=%u", param->max_tx);
    }
    if (param->mtu != 0) {
        snprintf(mtu_str, 32, ",mtu=%u", param->mtu);
    }
    return snprintf(desc, len, "uio,path=%s%s%s", param->path, max_tx_str, mtu_str);
}

static shmem_buffer_t *uio_buffer_init(unsigned short region, int fd) {
    shmem_buffer_t *uio_buffer = mmap(NULL, buffer_size(),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, region * getpagesize());

    if (uio_buffer == MAP_FAILED) {
        return NULL;
    }

    uio_buffer->size = buffer_size() - sizeof(shmem_buffer_t);
    return uio_buffer;
}

int pirate_internal_uio_open(void *_param, void *_ctx) {
    pirate_uio_param_t *param = (pirate_uio_param_t *)_param;
    uio_ctx *ctx = (uio_ctx *)_ctx;
    int err;
    uint_fast64_t init_pid = 0;
    shmem_buffer_t* buf;
    int access = ctx->flags & O_ACCMODE;

    pirate_uio_init_param(param);
    ctx->fd = open(param->path, O_RDWR | O_SYNC);
    if (ctx->fd < 0) {
        ctx->buf = NULL;
        return -1;
    }

    buf = uio_buffer_init(param->region, ctx->fd);
    ctx->buf = buf;
    if (ctx->buf == NULL) {
        goto error;
    }

    if (access == O_RDONLY) {
        if (!atomic_compare_exchange_strong(&buf->reader_pid, &init_pid,
                                            (uint64_t)getpid())) {
            errno = EBUSY;
            goto error;
        }

        do {
            init_pid = atomic_load(&buf->writer_pid);
        } while (!init_pid);
    } else {
        if (!atomic_compare_exchange_strong(&buf->writer_pid, &init_pid,
                                            (uint64_t)getpid())) {
            errno = EBUSY;
            goto error;
        }

        do {
            init_pid = atomic_load(&buf->reader_pid);
        } while (!init_pid);
    }

    return pirate_next_gd();
error:
    err = errno;
    close(ctx->fd);
    ctx->fd = -1;
    if (buf != NULL) {
        munmap(buf, buffer_size());
        ctx->buf = NULL;
    }
    errno = err;
    return -1;
}

int pirate_internal_uio_close(void *_ctx) {
    uio_ctx *ctx = (uio_ctx *)_ctx;
    shmem_buffer_t* buf = ctx->buf;
    int access = ctx->flags & O_ACCMODE;

    if (buf == NULL) {
        errno = EBADF;
        return -1;
    }

    if (access == O_RDONLY) {
        atomic_store(&buf->reader_pid, 0);
    } else {
        atomic_store(&buf->writer_pid, 0);
    }

    close(ctx->fd);
    ctx->fd = -1;
    return munmap(buf, buffer_size());
}

ssize_t pirate_internal_uio_read(const void *_param, void *_ctx, void *buffer, size_t count) {
    (void)_param;
    uio_ctx *ctx = (uio_ctx *)_ctx;
    uint64_t position;
    uint32_t reader, writer;
    size_t nbytes, nbytes1, nbytes2;
    int buffer_size;

    shmem_buffer_t* buf = ctx->buf;
    if (buf == NULL) {
        errno = EBADF;
        return -1;
    }

    for (;;) {
        position = atomic_load(&buf->position);
        if (!is_empty(position)) {
            break;
        }

        if (atomic_load(&buf->writer_pid) == 0) {
            return 0;
        }
    }

    reader = get_read(position);
    writer = get_write(position);
    buffer_size = buf->size;

    if (reader < writer) {
        nbytes = writer - reader;
    } else {
        nbytes = buffer_size + writer - reader;
    }

    count = MIN(count, 65536);
    nbytes = MIN(nbytes, count);
    nbytes1 = MIN(buffer_size - reader, nbytes);
    nbytes2 = nbytes - nbytes1;
    atomic_thread_fence(memory_order_acquire);
    memcpy(buffer, shared_buffer(buf) + reader, nbytes1);

    if (nbytes2 > 0) {
        memcpy(((char *)buffer) + nbytes1, shared_buffer(buf), nbytes2);
    }

    for (;;) {
        uint64_t update = create_position(writer,
                                        (reader + nbytes) % buffer_size, 0);
        if (atomic_compare_exchange_weak(&buf->position, &position,
                                            update)) {
            break;
        }
        writer = get_write(position);
    }

    return nbytes;
}

ssize_t pirate_internal_uio_write_mtu(const void *_param, void *_ctx) {
    (void) _ctx;
    const pirate_uio_param_t *param = (const pirate_uio_param_t *)_param;
    size_t mtu = param->mtu;
    if (mtu == 0) {
        return 0;
    }
    if (mtu < sizeof(pirate_header_t)) {
        errno = EINVAL;
        return -1;
    }
    return mtu - sizeof(pirate_header_t);
}

ssize_t pirate_internal_uio_write(const void *_param, void *_ctx, const void *buffer, size_t count) {
    (void)_param;
    uio_ctx *ctx = (uio_ctx *)_ctx;
    int buffer_size;
    size_t nbytes, nbytes1, nbytes2;
    uint64_t position;
    uint32_t reader, writer;

    shmem_buffer_t* buf = ctx->buf;
    if (buf == NULL) {
        errno = EBADF;
        return -1;
    }

    // The writer returns -1 when the reader has closed the channel.
    // The reader returns 0 when the writer has closed the channel AND
    // the channel is empty.
    do {
        if (atomic_load(&buf->reader_pid) == 0) {
            return -1;
        }
        position = atomic_load(&buf->position);
    } while (is_full(position));

    do {
        position = atomic_load(&buf->position);
    } while (is_full(position));

    reader = get_read(position);
    writer = get_write(position);
    buffer_size = buf->size;

    if (writer < reader) {
        nbytes = reader - writer;
    } else {
        nbytes = buffer_size + reader - writer;
    }

    count = MIN(count, 65536);
    nbytes = MIN(nbytes, count);
    nbytes1 = MIN(buffer_size - writer, nbytes);
    nbytes2 = nbytes - nbytes1;
    memcpy(shared_buffer(buf) + writer, buffer, nbytes1);

    if (nbytes2 > 0) {
        memcpy(shared_buffer(buf), ((char *)buffer) + nbytes1, nbytes2);
    }

    atomic_thread_fence(memory_order_release);
    for (;;) {
        uint64_t update = create_position((writer + nbytes) % buffer_size,
                                            reader, 1);
        if (atomic_compare_exchange_weak(&buf->position, &position,
                                            update)) {
            break;
        }
        reader = get_read(position);
    }
    return nbytes;
}
