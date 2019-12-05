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

#include "uio.h"

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

static inline int buffer_size() { return getpagesize() * 16; }

static shmem_buffer_t *uio_buffer_init(int gd, int fd) {
  shmem_buffer_t *uio_buffer;

  uio_buffer = mmap(NULL, buffer_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    gd * getpagesize());

  if (uio_buffer == MAP_FAILED) {
    return NULL;
  }

  uio_buffer->size = buffer_size() - sizeof(shmem_buffer_t);

  return uio_buffer;
}

int uio_buffer_open(int gd, int flags, pirate_channel_t *channels) {
  atomic_uint_fast64_t init_pid;
  int err, fd;
  shmem_buffer_t *uio_buffer;

  if (gd > 3) {
    errno = ENODEV;
    return -1;
  }

  fd = open("/dev/uio0", O_RDWR | O_SYNC);
  if (fd < 0) {
    channels[gd].shmem_buffer = NULL;
    return -1;
  }
  uio_buffer = uio_buffer_init(gd, fd);
  if (uio_buffer == NULL) {
    goto error;
  }
  channels[gd].shmem_buffer = uio_buffer;
  init_pid = 0;
  if (flags == O_RDONLY) {
    if (!atomic_compare_exchange_strong(&uio_buffer->reader_pid, &init_pid,
                                        (uint64_t)getpid())) {
      errno = EBUSY;
      goto error;
    }
    do {
      init_pid = atomic_load(&uio_buffer->writer_pid);
    } while (!init_pid);
  } else {
    if (!atomic_compare_exchange_strong(&uio_buffer->writer_pid, &init_pid,
                                        (uint64_t)getpid())) {
      errno = EBUSY;
      goto error;
    }
    do {
      init_pid = atomic_load(&uio_buffer->reader_pid);
    } while (!init_pid);
  }
  return fd;
error:
  err = errno;
  close(fd);
  if (uio_buffer != NULL) {
    munmap(uio_buffer, buffer_size());
  }
  channels[gd].shmem_buffer = NULL;
  channels[gd].fd = 0;
  errno = err;
  return -1;
}

ssize_t uio_buffer_read(shmem_buffer_t *uio_buffer, void *buf, size_t count) {
  int buffer_size;
  size_t nbytes, nbytes1, nbytes2;
  uint64_t position;
  uint32_t reader, writer;

  if (uio_buffer == NULL) {
    errno = EBADF;
    return -1;
  }

  for (;;) {
    position = atomic_load(&uio_buffer->position);
    if (!is_empty(position)) {
      break;
    }
    if (atomic_load(&uio_buffer->writer_pid) == 0) {
      return 0;
    }
  }

  reader = get_read(position);
  writer = get_write(position);
  buffer_size = uio_buffer->size;
  if (reader < writer) {
    nbytes = writer - reader;
  } else {
    nbytes = buffer_size + writer - reader;
  }
  nbytes = MIN(nbytes, count);
  nbytes1 = MIN(buffer_size - reader, nbytes);
  nbytes2 = nbytes - nbytes1;
  atomic_thread_fence(memory_order_acquire);
  memcpy(buf, uio_buffer->buffer + reader, nbytes1);
  if (nbytes2 > 0) {
    memcpy(((char *)buf) + nbytes1, uio_buffer->buffer + nbytes1, nbytes2);
  }
  for (;;) {
    uint64_t update =
        create_position(writer, (reader + nbytes) % buffer_size, 0);
    if (atomic_compare_exchange_weak(&uio_buffer->position, &position,
                                     update)) {
      break;
    }
    writer = get_write(position);
  }
  return nbytes;
}

ssize_t uio_buffer_write(shmem_buffer_t *uio_buffer, const void *buf,
                         size_t size) {
  int buffer_size;
  size_t nbytes, nbytes1, nbytes2;
  uint64_t position;
  uint32_t reader, writer;

  if (uio_buffer == NULL) {
    errno = EBADF;
    return -1;
  }

  // The writer returns -1 when the reader has closed the channel.
  // The reader returns 0 when the writer has closed the channel AND
  // the channel is empty.
  do {
    if (atomic_load(&uio_buffer->reader_pid) == 0) {
      return -1;
    }
    position = atomic_load(&uio_buffer->position);
  } while (is_full(position));

  do {
    position = atomic_load(&uio_buffer->position);
  } while (is_full(position));

  reader = get_read(position);
  writer = get_write(position);
  buffer_size = uio_buffer->size;
  if (writer < reader) {
    nbytes = reader - writer;
  } else {
    nbytes = buffer_size + reader - writer;
  }
  nbytes = MIN(nbytes, size);
  nbytes1 = MIN(buffer_size - writer, nbytes);
  nbytes2 = nbytes - nbytes1;
  memcpy(uio_buffer->buffer + writer, buf, nbytes1);
  if (nbytes2 > 0) {
    memcpy(uio_buffer->buffer, ((char *)buf) + nbytes1, nbytes2);
  }
  atomic_thread_fence(memory_order_release);
  for (;;) {
    uint64_t update =
        create_position((writer + nbytes) % buffer_size, reader, 1);
    if (atomic_compare_exchange_weak(&uio_buffer->position, &position,
                                     update)) {
      break;
    }
    reader = get_read(position);
  }
  return nbytes;
}

int uio_buffer_close(int flags, shmem_buffer_t *uio_buffer) {
  if (uio_buffer == NULL) {
    errno = EBADF;
    return -1;
  }
  if (flags == O_RDONLY) {
    atomic_store(&uio_buffer->reader_pid, 0);
  } else {
    atomic_store(&uio_buffer->writer_pid, 0);
  }
  return munmap(uio_buffer, buffer_size());
}
