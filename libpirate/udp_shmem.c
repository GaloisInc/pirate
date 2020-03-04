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
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "checksum.h"
#include "pirate_common.h"
#include "shmem_buffer.h"
#include "udp_shmem.h"

struct ip_hdr {
    uint8_t  version : 4;
    uint8_t  ihl : 4;
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags : 3;
    uint16_t frag_offset : 13;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t csum;
    uint32_t srcaddr;
    uint32_t dstaddr;
} __attribute__((packed));

struct udp_hdr {
    uint16_t srcport;
    uint16_t dstport;
    uint16_t len;
    uint16_t csum;
} __attribute__((packed));

struct pseudo_ip_hdr {
    uint32_t srcaddr;
    uint32_t dstaddr;
    uint8_t  zeros;
    uint8_t  proto;
    uint16_t udp_len;
    uint16_t srcport;
    uint16_t dstport;
    uint16_t len;
    uint16_t csum;
} __attribute__((packed));

#define UDP_HEADER_SIZE (sizeof(struct ip_hdr) + sizeof(struct udp_hdr))
#define SPIN_ITERATIONS 100000

#define IPV4(A, B, C, D)                                                       \
  ((uint32_t)(((A)&0xff) << 24) | (((B)&0xff) << 16) | (((C)&0xff) << 8) |     \
   ((D)&0xff))

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

static inline int is_full(uint64_t value) { return get_status(value) == 2; }


int udp_shmem_buffer_init_param(int gd, int flags,
                                pirate_udp_shmem_param_t *param) {
    (void) flags;
    snprintf(param->path, PIRATE_SHMEM_LEN_NAME - 1,  
              PIRATE_SHMEM_NAME, gd);
    param->buffer_size = DEFAULT_SMEM_BUF_LEN;
    param->packet_size = DEFAULT_UDP_SHMEM_PACKET_SIZE;
    param->packet_count = DEFAULT_UDP_SHMEM_PACKET_COUNT;
    return 0;
}

int udp_shmem_buffer_parse_param(int gd, int flags, char *str,
                                    pirate_udp_shmem_param_t *param) {
    char *ptr = NULL;

    if (udp_shmem_buffer_init_param(gd, flags, param) != 0) {
        return -1;
    }

    if (((ptr = strtok(str, OPT_DELIM)) == NULL) || 
        (strcmp(ptr, "udp_shmem") != 0)) {
        return -1;
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        strncpy(param->path, ptr, sizeof(param->path));
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->buffer_size = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->packet_size = strtol(ptr, NULL, 10);
    }

    if ((ptr = strtok(NULL, OPT_DELIM)) != NULL) {
        param->packet_count = strtol(ptr, NULL, 10);
    }

    return 0;
}

static int udp_shmem_buffer_init(int fd, pirate_udp_shmem_ctx_t *ctx) {
    int rv;
    int err;
    int success = 0;
    const int buffer_size = ctx->param.packet_size * ctx->param.packet_count;
    const size_t alloc_size = sizeof(shmem_buffer_t) + buffer_size;
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;

    if ((rv = ftruncate(fd, alloc_size)) != 0) {
        err = errno;
        close(fd);
        errno = err;
        return -1;
    }

    ctx->buf = (shmem_buffer_t *)mmap(NULL, alloc_size,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ctx->buf == MAP_FAILED) {
        err = errno;
        close(fd);
        errno = err;
        return -1;
    }

    close(fd);
    fd = -1;

    while (!success) {
        uint64_t init = atomic_load(&ctx->buf->init);
        switch (init) {
            
        case 0:
            if (atomic_compare_exchange_weak(&ctx->buf->init, &init, 1)) {
                success = 1;
            }
            break;
    
        case 1:
            // wait for initialization
            break;
        
        case 2:
            return 0;

        default:
            munmap(ctx->buf, alloc_size);
            return -1;
        }
    }

    ctx->buf->size = buffer_size;
    ctx->buf->buffer = (unsigned char *)ctx->buf + sizeof(shmem_buffer_t);
    ctx->buf->packet_size = ctx->param.packet_size;
    ctx->buf->packet_count = ctx->param.packet_count;

    if ((rv = sem_init(&ctx->buf->reader_open_wait, 1, 0)) != 0) {
        goto error;
    }

    if ((rv = sem_init(&ctx->buf->writer_open_wait, 1, 0)) != 0) {
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

    if ((rv = pthread_mutex_init(&ctx->buf->mutex, &mutex_attr)) != 0) {
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

    if ((rv = pthread_cond_init(&ctx->buf->is_not_empty, &cond_attr)) != 0) {
        errno = rv;
        goto error;
    }

    if ((rv = pthread_cond_init(&ctx->buf->is_not_full, &cond_attr)) != 0) {
        errno = rv;
        goto error;
    }

    atomic_store(&ctx->buf->init, 2);
    return 0;
error:
    err = errno;
    atomic_store(&ctx->buf->init, 0);
    munmap(ctx->buf, alloc_size);
    ctx->buf = NULL;
    errno = err;
    return -1;
}

int udp_shmem_buffer_open(int gd, int flags, pirate_udp_shmem_ctx_t *ctx) {
    int err;
    uint_fast64_t init_pid = 0;

    // on successful shm_open (fd > 0) we must shm_unlink before exiting
    // this function
    int fd = shm_open(ctx->param.path, O_RDWR | O_CREAT, 0660);
    if (fd < 0) {
        ctx->buf = NULL;
        return -1;
    }

    if (udp_shmem_buffer_init(fd, ctx)) {
        goto error;
    }

    if (flags == O_RDONLY) {
        if (!atomic_compare_exchange_strong(&ctx->buf->reader_pid, &init_pid,
                                            (uint64_t)getpid())) {
            errno = EBUSY;
            goto error;
        }

        if (sem_post(&ctx->buf->writer_open_wait) < 0) {
            atomic_store(&ctx->buf->reader_pid, 0);
            goto error;
        }

        if (sem_wait(&ctx->buf->reader_open_wait) < 0) {
            atomic_store(&ctx->buf->reader_pid, 0);
            goto error;
        }
    } else {
        if (!atomic_compare_exchange_strong(&ctx->buf->writer_pid, &init_pid,
                                            (uint64_t)getpid())) {
            errno = EBUSY;
        goto error;
        }

        if (sem_post(&ctx->buf->reader_open_wait) < 0) {
            atomic_store(&ctx->buf->writer_pid, 0);
            goto error;
        }

        if (sem_wait(&ctx->buf->writer_open_wait) < 0) {
            atomic_store(&ctx->buf->writer_pid, 0);
            goto error;
        }
  }
    if (shm_unlink(ctx->param.path) == -1) {
        if (errno == ENOENT) {
            errno = 0;
        } else {
            goto error;
        }
    }

    ctx->flags = flags;
    return gd;
error:
    err = errno;
    ctx->buf = NULL;
    shm_unlink(ctx->param.path);
    errno = err;
    return -1;
}

int udp_shmem_buffer_close(pirate_udp_shmem_ctx_t *ctx) {
  const size_t alloc_size = sizeof(shmem_buffer_t) + ctx->buf->size;

    if (ctx->flags == O_RDONLY) {
        atomic_store(&ctx->buf->reader_pid, 0);
        pthread_mutex_lock(&ctx->buf->mutex);
        pthread_cond_signal(&ctx->buf->is_not_full);
        pthread_mutex_unlock(&ctx->buf->mutex);
    } else {
        atomic_store(&ctx->buf->writer_pid, 0);
        pthread_mutex_lock(&ctx->buf->mutex);
        pthread_cond_signal(&ctx->buf->is_not_empty);
        pthread_mutex_unlock(&ctx->buf->mutex);
    }

    return munmap(ctx->buf, alloc_size);
}

ssize_t udp_shmem_buffer_read(pirate_udp_shmem_ctx_t *ctx, void *buf, 
                                size_t count) {
    uint64_t position;
    int was_full;
    uint32_t reader, writer;
    struct ip_hdr ip_header;
    struct udp_hdr udp_header;
    uint16_t exp_csum, obs_csum;
    struct pseudo_ip_hdr pseudo_header;

    if (ctx->buf == NULL) {
        errno = EBADF;
        return -1;
    }

    const size_t packet_size = ctx->buf->packet_size;
    const size_t packet_count = ctx->buf->packet_count;

    if (count > (packet_size - UDP_HEADER_SIZE)) {
        count = packet_size - UDP_HEADER_SIZE;
    }

    position = atomic_load(&ctx->buf->position);
    for (int spin = 0; (spin < SPIN_ITERATIONS) && is_empty(position); spin++) {
        position = atomic_load(&ctx->buf->position);
    }

    if (is_empty(position)) {
        pthread_mutex_lock(&ctx->buf->mutex);
        position = atomic_load(&ctx->buf->position);
        while (is_empty(position)) {
            // The reader returns 0 when the writer has closed
            // the channel and the channel is empty. If the writer
            // has closed the channel and the buffer has content
            // then return the contents of the buffer.
            if (atomic_load(&ctx->buf->writer_pid) == 0) {
                pthread_mutex_unlock(&ctx->buf->mutex);
                return 0;
            }
            pthread_cond_wait(&ctx->buf->is_not_empty, &ctx->buf->mutex);
            position = atomic_load(&ctx->buf->position);
        }
        pthread_mutex_unlock(&ctx->buf->mutex);
    }

    reader = get_read(position);
    writer = get_write(position);

    atomic_thread_fence(memory_order_acquire);
    memcpy(&ip_header, ctx->buf->buffer + (reader * packet_size),
            sizeof(struct ip_hdr));
    memcpy(&udp_header, ctx->buf->buffer + (reader * packet_size) + 
            sizeof(struct ip_hdr), sizeof(struct udp_hdr));
    memcpy(buf, ctx->buf->buffer + (reader * packet_size) + 
            UDP_HEADER_SIZE, count);

    for (;;) {
        uint64_t update = create_position(writer, (reader + 1) % 
                            packet_count, 0);
        if (atomic_compare_exchange_weak(&ctx->buf->position, &position,
                                     update)) {
            was_full = is_full(position);
            break;
        }
        writer = get_write(position);
    }

    if (was_full) {
        pthread_mutex_lock(&ctx->buf->mutex);
        pthread_cond_signal(&ctx->buf->is_not_full);
        pthread_mutex_unlock(&ctx->buf->mutex);
    }

    exp_csum = ip_header.csum;
    ip_header.csum = 0;
    obs_csum = cksum_avx2((void*) &ip_header, sizeof(struct ip_hdr), 0);
    if (exp_csum != obs_csum) {
        errno = EL2HLT;
        return -1;
    }

    pseudo_header.srcaddr = ip_header.srcaddr;
    pseudo_header.dstaddr = ip_header.dstaddr;
    pseudo_header.zeros = 0;
    pseudo_header.proto = 17;
    pseudo_header.udp_len = udp_header.len;
    pseudo_header.srcport = udp_header.srcport;
    pseudo_header.dstport = udp_header.dstport;
    pseudo_header.len = udp_header.len;
    pseudo_header.csum = 0;

    exp_csum = udp_header.csum;
    obs_csum = cksum_avx2((void*) &pseudo_header, 
                            sizeof(struct pseudo_ip_hdr), 0);
    obs_csum = cksum_avx2((void*) buf, count, ~obs_csum);
    if (exp_csum != obs_csum) {
        errno = EL3HLT;
        return -1;
    }

  return count;
}

ssize_t udp_shmem_buffer_write(pirate_udp_shmem_ctx_t *ctx, const void *buf,
                            size_t count) {
    int was_empty;
    uint32_t reader, writer;
    uint64_t position;
    uint16_t csum;
    struct ip_hdr ip_header;
    struct udp_hdr udp_header;
    struct pseudo_ip_hdr pseudo_header;

    if (ctx->buf == NULL) {
        errno = EBADF;
        return -1;
    }

    const size_t packet_size = ctx->buf->packet_size;
    const size_t packet_count = ctx->buf->packet_count;

    if (count > (packet_size - UDP_HEADER_SIZE)) {
        count = packet_size - UDP_HEADER_SIZE;
    }

    memset(&ip_header, 0, sizeof(struct ip_hdr));
    ip_header.version = 4;
    ip_header.ihl = 5;
    ip_header.tos = 16; // low delay
    ip_header.len = count + UDP_HEADER_SIZE;
    ip_header.ttl = 1;
    ip_header.proto = 17; // UDP
    ip_header.srcaddr = IPV4(127, 0, 0, 1);
    ip_header.dstaddr = IPV4(127, 0, 0, 1);
    ip_header.csum = cksum_avx2((void*) &ip_header, sizeof(struct ip_hdr), 0);

    udp_header.srcport = 0;
    udp_header.dstport = 0;
    udp_header.len = count + sizeof(struct udp_hdr);
    udp_header.csum = 0;

    pseudo_header.srcaddr = ip_header.srcaddr;
    pseudo_header.dstaddr = ip_header.dstaddr;
    pseudo_header.zeros = 0;
    pseudo_header.proto = 17;
    pseudo_header.udp_len = udp_header.len;
    pseudo_header.srcport = udp_header.srcport;
    pseudo_header.dstport = udp_header.dstport;
    pseudo_header.len = udp_header.len;
    pseudo_header.csum = 0;

    csum = cksum_avx2((void*) &pseudo_header, sizeof(struct pseudo_ip_hdr), 0);
    csum = cksum_avx2((void*) buf, count, ~csum);
    udp_header.csum = csum;

    position = atomic_load(&ctx->buf->position);
    for (int spin = 0; (spin < SPIN_ITERATIONS) && is_full(position); spin++) {
        position = atomic_load(&ctx->buf->position);
    }

    if (is_full(position)) {
        pthread_mutex_lock(&ctx->buf->mutex);
        position = atomic_load(&ctx->buf->position);
        while (is_full(position)) {
            if (atomic_load(&ctx->buf->reader_pid) == 0) {
                pthread_mutex_unlock(&ctx->buf->mutex);
                kill(getpid(), SIGPIPE);
                errno = EPIPE;
                return -1;
            }
            pthread_cond_wait(&ctx->buf->is_not_full, &ctx->buf->mutex);
            position = atomic_load(&ctx->buf->position);
        }
        pthread_mutex_unlock(&ctx->buf->mutex);
    }

    // The writer returns -1 when the reader has closed the channel.
    // The reader returns 0 when the writer has closed the channel AND
    // the channel is empty.
    if (atomic_load(&ctx->buf->reader_pid) == 0) {
        kill(getpid(), SIGPIPE);
        errno = EPIPE;
        return -1;
    }
    reader = get_read(position);
    writer = get_write(position);

    memcpy(ctx->buf->buffer + (writer * packet_size), &ip_header,
            sizeof(struct ip_hdr));
    memcpy(ctx->buf->buffer + (writer * packet_size) + sizeof(struct ip_hdr),
            &udp_header, sizeof(struct udp_hdr));
    memcpy(ctx->buf->buffer + (writer * packet_size) + UDP_HEADER_SIZE, buf,
            count);
    atomic_thread_fence(memory_order_release);

    for (;;) {
        uint64_t update = create_position((writer + 1) % packet_count, 
                                            reader, 1);
        if (atomic_compare_exchange_weak(&ctx->buf->position, &position,
                                            update)) {
            was_empty = is_empty(position);
            break;
        }
        reader = get_read(position);
    }

    if (was_empty) {
        pthread_mutex_lock(&ctx->buf->mutex);
        pthread_cond_signal(&ctx->buf->is_not_empty);
        pthread_mutex_unlock(&ctx->buf->mutex);
    }

    return count;
}
