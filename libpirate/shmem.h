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

#ifndef __SHMEM_H
#define __SHMEM_H

#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
  atomic_uint_fast64_t position;
  atomic_uint_fast64_t init;
  atomic_uint_fast64_t reader_pid;
  atomic_uint_fast64_t writer_pid;
  sem_t reader_open_wait;
  sem_t writer_open_wait;
  pthread_mutex_t mutex;
  pthread_cond_t is_not_empty;
  pthread_cond_t is_not_full;
  int size;
  char buffer[];
} shmem_buffer_t;

#define DEFAULT_SHMEM_BUFFER (131072)

int shmem_buffer_open(int gd, int flags, int size, char *name, shmem_buffer_t **buffer_addr);

ssize_t shmem_buffer_read(shmem_buffer_t *shmem_buffer, void *buf, size_t count);

ssize_t shmem_buffer_write(shmem_buffer_t *shmem_buffer, const void *buf, size_t size);

int shmem_buffer_close(int flags, shmem_buffer_t *shmem_buffer);

#endif
