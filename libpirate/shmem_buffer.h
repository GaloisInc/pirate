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

#ifndef __PIRATE_SHMEM_BUFFER_H
#define __PIRATE_SHMEM_BUFFER_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
#include <atomic>
typedef std::atomic_uint_fast64_t pirate_atomic_uint64;
#elif HAVE_STD_ATOMIC
#include <stdatomic.h>
typedef atomic_uint_fast64_t pirate_atomic_uint64;
#else
// This is a workaround.
// Do not use shmem_buffer_t when atomics are not available
typedef uint_fast64_t pirate_atomic_uint64;
#endif

typedef struct {
    pirate_atomic_uint64    position;
    pirate_atomic_uint64    init;
    pirate_atomic_uint64    reader_pid;
    pirate_atomic_uint64    writer_pid;
    sem_t                   reader_open_wait;
    sem_t                   writer_open_wait;
    pthread_mutex_t         mutex;
    pthread_cond_t          is_not_empty;
    pthread_cond_t          is_not_full;
    int                     size;
    size_t                  packet_size;
    size_t                  packet_count;
} shmem_buffer_t;

#endif /* __PIRATE_SHMEM_BUFFER_H */
