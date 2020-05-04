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

#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "pirate_common.h"
#include "shmem.h"

#include <stdio.h>

#define SPIN_ITERATIONS 100000

// TODO: replace reader and writer with BipBuffer
// https://ferrous-systems.com/blog/lock-free-ring-buffer/
// use 20 bits for reader, 20 bits for writer, 20 bits for watermark
// use remaining 4 bits for status
// use status bits to indicate whether empty or full

static inline uint8_t get_status(uint64_t value) {
    return (value & 0xff00000000000000) >> 56;
}

static inline uint32_t get_write(uint64_t value) {
    return (value & 0x00fffffff0000000) >> 28;
}

static inline uint32_t get_read(uint64_t value) {
    return (value & 0x000000000fffffff);
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

static inline unsigned char* shared_buffer(shmem_buffer_t *shmem_buffer) {
    return (unsigned char *)shmem_buffer + sizeof(shmem_buffer_t);
}

static shmem_buffer_t *shmem_buffer_init(int fd, int buffer_size) {
    int err, rv;
    int success = 0;
    shmem_buffer_t *shmem_buffer = NULL;
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    const size_t alloc_size = sizeof(shmem_buffer_t) + buffer_size;
    if ((rv = ftruncate(fd, alloc_size)) != 0) {
        err = errno;
        close(fd);
        errno = err;
        return NULL;
    }

    shmem_buffer = (shmem_buffer_t *)mmap(NULL, alloc_size,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (shmem_buffer == MAP_FAILED) {
        err = errno;
        close(fd);
        errno = err;
        return NULL;
    }

    close(fd);
    fd = -1;

    while (!success) {
        uint64_t init = atomic_load(&shmem_buffer->init);
        switch (init) {
        
        case 0:
            if (atomic_compare_exchange_weak(&shmem_buffer->init, &init, 1)) {
                success = 1;
            }
            break;
    
        case 1:
            // wait for initialization
            break;
        
        case 2:
            return shmem_buffer;

        default:
            munmap(shmem_buffer, alloc_size);
            return NULL;
        }
    }

    shmem_buffer->size = buffer_size;

    if ((rv = sem_init(&shmem_buffer->reader_open_wait, 1, 0)) != 0) {
        goto error;
    }

    if ((rv = sem_init(&shmem_buffer->writer_open_wait, 1, 0)) != 0) {
        goto error;
    }

    if ((rv = pthread_mutexattr_init(&mutex_attr)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_mutexattr_setpshared(&mutex_attr,
                                         PTHREAD_PROCESS_SHARED)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_mutex_init(&shmem_buffer->mutex, &mutex_attr)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_condattr_init(&cond_attr)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_condattr_setpshared(&cond_attr,
                                        PTHREAD_PROCESS_SHARED)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_cond_init(&shmem_buffer->is_not_empty,
                                        &cond_attr)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_cond_init(&shmem_buffer->is_not_full, &cond_attr)) != 0) {
        errno = rv;
        goto error;
    }

    atomic_store(&shmem_buffer->init, 2);
    return shmem_buffer;
error:
    err = errno;
    atomic_store(&shmem_buffer->init, 0);
    munmap(shmem_buffer, alloc_size);
    errno = err;
    return NULL;
}

static void shmem_buffer_init_param(pirate_shmem_param_t *param) {
    if (param->buffer_size == 0) {
        param->buffer_size = DEFAULT_SMEM_BUF_LEN;
    }
}

int shmem_buffer_parse_param(char *str, pirate_shmem_param_t *param) {
    char *ptr = NULL, *key, *val;
    char *saveptr1, *saveptr2;

    if (((ptr = strtok_r(str, OPT_DELIM, &saveptr1)) == NULL) ||
        (strcmp(ptr, "shmem") != 0)) {
        return -1;
    }

    if ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) == NULL) {
        errno = EINVAL;
        return -1;
    }
    strncpy(param->path, ptr, sizeof(param->path) - 1);

    while ((ptr = strtok_r(NULL, OPT_DELIM, &saveptr1)) != NULL) {
        int rv = pirate_parse_key_value(&key, &val, ptr, &saveptr2);
        if (rv < 0) {
            return rv;
        } else if (rv == 0) {
            continue;
        }
        if (strncmp("buffer_size", key, strlen("buffer_size")) == 0) {
            param->buffer_size = strtol(val, NULL, 10);
        } else {
            errno = EINVAL;
            return -1;
        }
    }
    return 0;
}

int shmem_buffer_get_channel_description(const pirate_shmem_param_t *param, char *desc, int len) {
    return snprintf(desc, len - 1, "shmem,%s,buffer_size=%u", param->path,
                    param->buffer_size);
}

int shmem_buffer_open(int flags, pirate_shmem_param_t *param, shmem_ctx *ctx) {
    int err;
    uint_fast64_t init_pid = 0;
    shmem_buffer_t* buf;
    int access = flags & O_ACCMODE;

    shmem_buffer_init_param(param);
    // on successful shm_open (fd > 0) we must shm_unlink before exiting
    // this function
    int fd = shm_open(param->path, O_RDWR | O_CREAT, 0660);
    if (fd < 0) {
        ctx->buf = NULL;
        return -1;
    }

    buf = shmem_buffer_init(fd, param->buffer_size);
    ctx->buf = buf;
    if (ctx->buf == NULL) {
        goto error;
    }

    if (access == O_RDONLY) {
        if (!atomic_compare_exchange_strong(&buf->reader_pid,
                                            &init_pid, (uint64_t)getpid())) {
            errno = EBUSY;
            goto error;
        }

        if (sem_post(&buf->writer_open_wait) < 0) {
            atomic_store(&buf->reader_pid, 0);
            goto error;
        }

        if (sem_wait(&buf->reader_open_wait) < 0) {
            atomic_store(&buf->reader_pid, 0);
            goto error;
        }
    } else {
        if (!atomic_compare_exchange_strong(&buf->writer_pid, &init_pid,
                                        (uint64_t)getpid())) {
            errno = EBUSY;
            goto error;
        }

        if (sem_post(&buf->reader_open_wait) < 0) {
            atomic_store(&buf->writer_pid, 0);
            goto error;
        }

        if (sem_wait(&buf->writer_open_wait) < 0) {
            atomic_store(&buf->writer_pid, 0);
            goto error;
        }
    }
    err = errno;
    if (shm_unlink(param->path) == -1) {
        if (errno == ENOENT) {
            errno = err;
        } else {
            goto error;
        }
    }

    ctx->flags = flags;
    return 0;
error:
    err = errno;
    ctx->buf = NULL;
    shm_unlink(param->path);
    errno = err;
    return -1;
}

int shmem_buffer_close(shmem_ctx *ctx) {
    shmem_buffer_t* buf = ctx->buf;
    const size_t alloc_size = sizeof(shmem_buffer_t) + buf->size;
    int access = ctx->flags & O_ACCMODE;

    if (access == O_RDONLY) {
        atomic_store(&buf->reader_pid, 0);
        pthread_mutex_lock(&buf->mutex);
        pthread_cond_signal(&buf->is_not_full);
        pthread_mutex_unlock(&buf->mutex);
    } else {
        atomic_store(&buf->writer_pid, 0);
        pthread_mutex_lock(&buf->mutex);
        pthread_cond_signal(&buf->is_not_empty);
        pthread_mutex_unlock(&buf->mutex);
    }

    return munmap(buf, alloc_size);
}

ssize_t shmem_buffer_read(const pirate_shmem_param_t *param, shmem_ctx *ctx, void *buffer, size_t count) {
    (void) param;
    uint64_t position;
    int was_full;
    size_t nbytes, nbytes1, nbytes2;
    uint32_t reader, writer;

    shmem_buffer_t* buf = ctx->buf;
    if (buf == NULL) {
        errno = EBADF;
        return -1;
    }

    position = atomic_load(&buf->position);
    for (int spin = 0; (spin < SPIN_ITERATIONS) && is_empty(position); spin++) {
        position = atomic_load(&buf->position);
    }

    if (is_empty(position)) {
        pthread_mutex_lock(&buf->mutex);
        position = atomic_load(&buf->position);
        while (is_empty(position)) {
            // The reader returns 0 when the writer has closed
            // the channel and the channel is empty. If the writer
            // has closed the channel and the buffer has content
            // then return the contents of the buffer.
            if (atomic_load(&buf->writer_pid) == 0) {
                pthread_mutex_unlock(&buf->mutex);
                return 0;
            }
            pthread_cond_wait(&buf->is_not_empty, &buf->mutex);
            position = atomic_load(&buf->position);
        }
        pthread_mutex_unlock(&buf->mutex);
    }

    reader = get_read(position);
    writer = get_write(position);

    if (reader < writer) {
        nbytes = writer - reader;
    } else {
        nbytes = buf->size + writer - reader;
    }

    count = MIN(count, 65536);
    nbytes = MIN(nbytes, count);
    nbytes1 = MIN(buf->size - reader, nbytes);
    nbytes2 = nbytes - nbytes1;
    atomic_thread_fence(memory_order_acquire);
    memcpy(buffer, shared_buffer(buf) + reader, nbytes1);
    if (nbytes2 > 0) {
        memcpy(((char *)buffer) + nbytes1, shared_buffer(buf), nbytes2);
    }

    for (;;) {
        uint64_t update = create_position(writer,
            (reader + nbytes) % buf->size, 0);
        if (atomic_compare_exchange_weak(&buf->position, &position,
                update)) {
            was_full = is_full(position);
            break;
        }
        writer = get_write(position);
    }

    if (was_full) {
        pthread_mutex_lock(&buf->mutex);
        pthread_cond_signal(&buf->is_not_full);
        pthread_mutex_unlock(&buf->mutex);
    }

    return nbytes;
}

ssize_t shmem_buffer_write(const pirate_shmem_param_t *param, shmem_ctx *ctx, const void *buffer,
                            size_t count) {
    (void) param;
    uint64_t position;
    int was_empty;
    size_t nbytes, nbytes1, nbytes2;
    uint32_t reader, writer;

    shmem_buffer_t* buf = ctx->buf;
    if (buf == NULL) {
        errno = EBADF;
        return -1;
    }

    position = atomic_load(&buf->position);
    for (int spin = 0; (spin < SPIN_ITERATIONS) && is_full(position); spin++) {
        position = atomic_load(&buf->position);
    }

    if (is_full(position)) {
        pthread_mutex_lock(&buf->mutex);
        position = atomic_load(&buf->position);
        while (is_full(position)) {
            if (atomic_load(&buf->reader_pid) == 0) {
                pthread_mutex_unlock(&buf->mutex);
                kill(getpid(), SIGPIPE);
                errno = EPIPE;
                return -1;
            }
            pthread_cond_wait(&buf->is_not_full, &buf->mutex);
            position = atomic_load(&buf->position);
        }

        pthread_mutex_unlock(&buf->mutex);
    }

    // The writer returns -1 when the reader has closed the channel.
    // The reader returns 0 when the writer has closed the channel AND
    // the channel is empty.
    if (atomic_load(&buf->reader_pid) == 0) {
        kill(getpid(), SIGPIPE);
        errno = EPIPE;
        return -1;
    }

    reader = get_read(position);
    writer = get_write(position);
    if (writer < reader) {
        nbytes = reader - writer;
    } else {
        nbytes = buf->size + reader - writer;
    }

    count = MIN(count, 65536);
    nbytes = MIN(nbytes, count);
    nbytes1 = MIN(buf->size - writer, nbytes);
    nbytes2 = nbytes - nbytes1;
    memcpy(shared_buffer(buf) + writer, buffer, nbytes1);
    if (nbytes2 > 0) {
        memcpy(shared_buffer(buf), ((char *)buffer) + nbytes1, nbytes2);
    }
    atomic_thread_fence(memory_order_release);
    for (;;) {
        uint64_t update = create_position((writer + nbytes) % buf->size,
                                            reader, 1);
        if (atomic_compare_exchange_weak(&buf->position, &position,
                                            update)) {
            was_empty = is_empty(position);
            break;
        }
        reader = get_read(position);
    }

    if (was_empty) {
        pthread_mutex_lock(&buf->mutex);
        pthread_cond_signal(&buf->is_not_empty);
        pthread_mutex_unlock(&buf->mutex);
    }

    return nbytes;
}
