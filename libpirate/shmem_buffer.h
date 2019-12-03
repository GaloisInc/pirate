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

#ifndef __SHMEM_BUFFER_H
#define __SHMEM_BUFFER_H

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

#endif
