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


#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SPIN_ITERATIONS (100000)

#include "shmem.h"

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

static inline int is_empty(uint64_t value) { return get_status(value) == 0; }

static inline int is_full(uint64_t value) { return get_status(value) == 2; }

static shmem_buffer_t *shmem_buffer_init(int fd, int size) {
  int buffer_size, err, rv, success;
  size_t alloc_size;
  shmem_buffer_t *shmem_buffer;
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;

  if (size <= 0) {
    buffer_size = DEFAULT_SHMEM_BUFFER;
  } else {
    buffer_size = size;
  }
  alloc_size = sizeof(shmem_buffer_t) + buffer_size;
  if ((rv = ftruncate(fd, alloc_size)) != 0) {
    err = errno;
    close(fd);
    errno = err;
    return NULL;
  }

  shmem_buffer = (shmem_buffer_t *)mmap(
      NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (shmem_buffer == MAP_FAILED) {
    err = errno;
    close(fd);
    errno = err;
    return NULL;
  }

  close(fd);

  success = 0;
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

  if ((rv = pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED)) !=
      0) {
    errno = rv;
    goto error;
  }

  if ((rv = pthread_cond_init(&shmem_buffer->is_not_empty, &cond_attr)) != 0) {
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

int shmem_buffer_open(int gd, int flags, int size, char *name,
                      shmem_buffer_t **buffer_addr) {
  atomic_uint_fast64_t init_pid;
  int err, fd;
  shmem_buffer_t *shmem_buffer;

  init_pid = 0;
  // on successful shm_open (fd > 0) we must
  // shm_unlink before exiting this function
  fd = shm_open(name, O_RDWR | O_CREAT, 0660);
  if (fd < 0) {
    *buffer_addr = NULL;
    return -1;
  }
  shmem_buffer = shmem_buffer_init(fd, size);
  if (shmem_buffer == NULL) {
    goto error;
  }
  *buffer_addr = shmem_buffer;
  if (flags == O_RDONLY) {
    if (!atomic_compare_exchange_strong(&shmem_buffer->reader_pid, &init_pid,
                                        (uint64_t)getpid())) {
      errno = EBUSY;
      goto error;
    }
    if (sem_post(&shmem_buffer->writer_open_wait) < 0) {
      atomic_store(&shmem_buffer->reader_pid, 0);
      goto error;
    }
    if (sem_wait(&shmem_buffer->reader_open_wait) < 0) {
      atomic_store(&shmem_buffer->reader_pid, 0);
      goto error;
    }
  } else {
    if (!atomic_compare_exchange_strong(&shmem_buffer->writer_pid, &init_pid,
                                        (uint64_t)getpid())) {
      errno = EBUSY;
      goto error;
    }
    if (sem_post(&shmem_buffer->reader_open_wait) < 0) {
      atomic_store(&shmem_buffer->writer_pid, 0);
      goto error;
    }
    if (sem_wait(&shmem_buffer->writer_open_wait) < 0) {
      atomic_store(&shmem_buffer->writer_pid, 0);
      goto error;
    }
  }
  shm_unlink(name);
  return gd;
error:
  err = errno;
  *buffer_addr = NULL;
  shm_unlink(name);
  errno = err;
  return -1;
}

ssize_t shmem_buffer_read(shmem_buffer_t *shmem_buffer, void *buf,
                          size_t count) {
  int spin, buffer_size, was_full;
  size_t nbytes, nbytes1, nbytes2;
  uint64_t position;
  uint32_t reader, writer;

  position = atomic_load(&shmem_buffer->position);
  for (spin = 0; (spin < SPIN_ITERATIONS) && is_empty(position); spin++) {
    position = atomic_load(&shmem_buffer->position);
  }
  if (is_empty(position)) {
    pthread_mutex_lock(&shmem_buffer->mutex);
    position = atomic_load(&shmem_buffer->position);
    while (is_empty(position)) {
      // The reader returns 0 when the writer has closed
      // the channel and the channel is empty. If the writer
      // has closed the channel and the buffer has content
      // then return the contents of the buffer.
      if (atomic_load(&shmem_buffer->writer_pid) == 0) {
        pthread_mutex_unlock(&shmem_buffer->mutex);
        return 0;
      }
      pthread_cond_wait(&shmem_buffer->is_not_empty, &shmem_buffer->mutex);
      position = atomic_load(&shmem_buffer->position);
    }
    pthread_mutex_unlock(&shmem_buffer->mutex);
  }
  reader = get_read(position);
  writer = get_write(position);
  buffer_size = shmem_buffer->size;
  if (reader < writer) {
    nbytes = writer - reader;
  } else {
    nbytes = buffer_size + writer - reader;
  }
  nbytes = MIN(nbytes, count);
  nbytes1 = MIN(buffer_size - reader, nbytes);
  nbytes2 = nbytes - nbytes1;
  atomic_thread_fence(memory_order_acquire);
  memcpy(buf, shmem_buffer->buffer + reader, nbytes1);
  if (nbytes2 > 0) {
    memcpy(((char *)buf) + nbytes1, shmem_buffer->buffer + nbytes1, nbytes2);
  }
  for (;;) {
    uint64_t update =
        create_position(writer, (reader + nbytes) % buffer_size, 0);
    if (atomic_compare_exchange_weak(&shmem_buffer->position, &position,
                                     update)) {
      was_full = is_full(position);
      break;
    }
    writer = get_write(position);
  }
  if (was_full) {
    pthread_mutex_lock(&shmem_buffer->mutex);
    pthread_cond_signal(&shmem_buffer->is_not_full);
    pthread_mutex_unlock(&shmem_buffer->mutex);
  }
  return nbytes;
}

ssize_t shmem_buffer_write(shmem_buffer_t *shmem_buffer, const void *buf,
                           size_t size) {
  int spin, buffer_size, was_empty;
  size_t nbytes, nbytes1, nbytes2;
  uint64_t position;
  uint32_t reader, writer;

  position = atomic_load(&shmem_buffer->position);
  for (spin = 0; (spin < SPIN_ITERATIONS) && is_full(position); spin++) {
    position = atomic_load(&shmem_buffer->position);
  }
  if (is_full(position)) {
    pthread_mutex_lock(&shmem_buffer->mutex);
    position = atomic_load(&shmem_buffer->position);
    while (is_full(position)) {
      if (atomic_load(&shmem_buffer->reader_pid) == 0) {
        pthread_mutex_unlock(&shmem_buffer->mutex);
        kill(getpid(), SIGPIPE);
        errno = EPIPE;
        return -1;
      }
      pthread_cond_wait(&shmem_buffer->is_not_full, &shmem_buffer->mutex);
      position = atomic_load(&shmem_buffer->position);
    }
    pthread_mutex_unlock(&shmem_buffer->mutex);
  }
  // The writer returns -1 when the reader has closed the channel.
  // The reader returns 0 when the writer has closed the channel AND
  // the channel is empty.
  if (atomic_load(&shmem_buffer->reader_pid) == 0) {
    kill(getpid(), SIGPIPE);
    errno = EPIPE;
    return -1;
  }
  reader = get_read(position);
  writer = get_write(position);
  buffer_size = shmem_buffer->size;
  if (writer < reader) {
    nbytes = reader - writer;
  } else {
    nbytes = buffer_size + reader - writer;
  }
  nbytes = MIN(nbytes, size);
  nbytes1 = MIN(buffer_size - writer, nbytes);
  nbytes2 = nbytes - nbytes1;
  memcpy(shmem_buffer->buffer + writer, buf, nbytes1);
  if (nbytes2 > 0) {
    memcpy(shmem_buffer->buffer, ((char *)buf) + nbytes1, nbytes2);
  }
  atomic_thread_fence(memory_order_release);
  for (;;) {
    uint64_t update =
        create_position((writer + nbytes) % buffer_size, reader, 1);
    if (atomic_compare_exchange_weak(&shmem_buffer->position, &position,
                                     update)) {
      was_empty = is_empty(position);
      break;
    }
    reader = get_read(position);
  }
  if (was_empty) {
    pthread_mutex_lock(&shmem_buffer->mutex);
    pthread_cond_signal(&shmem_buffer->is_not_empty);
    pthread_mutex_unlock(&shmem_buffer->mutex);
  }
  return nbytes;
}

int shmem_buffer_close(int flags, shmem_buffer_t *shmem_buffer) {
  size_t buffer_size = sizeof(shmem_buffer_t) + shmem_buffer->size;
  if (flags == O_RDONLY) {
    atomic_store(&shmem_buffer->reader_pid, 0);
    pthread_mutex_lock(&shmem_buffer->mutex);
    pthread_cond_signal(&shmem_buffer->is_not_full);
    pthread_mutex_unlock(&shmem_buffer->mutex);
  } else {
    atomic_store(&shmem_buffer->writer_pid, 0);
    pthread_mutex_lock(&shmem_buffer->mutex);
    pthread_cond_signal(&shmem_buffer->is_not_empty);
    pthread_mutex_unlock(&shmem_buffer->mutex);
  }
  return munmap(shmem_buffer, buffer_size);
}
