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

#include "shmem_udp.h"
#include "checksum.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define SPIN_ITERATIONS (100000)

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

static inline int is_empty(uint64_t value) { return get_status(value) == 0; }

static inline int is_full(uint64_t value) { return get_status(value) == 2; }

static shmem_buffer_t *shmem_udp_buffer_init(int fd, pirate_channel_t *channel) {
  int buffer_size, err, rv, success;
  size_t packet_size, packet_count;
  size_t alloc_size;
  shmem_buffer_t *shmem_buffer;
  pthread_mutexattr_t mutex_attr;
  pthread_condattr_t cond_attr;

  if (channel->packet_size == 0) {
    packet_size = DEFAULT_PACKET_SIZE;
  } else {
    packet_size = channel->packet_size + UDP_HEADER_SIZE;
  }

  if (channel->packet_count == 0) {
    packet_count = DEFAULT_PACKET_COUNT;
  } else {
    packet_count = channel->packet_count;
  }

  buffer_size = packet_size * packet_count;
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

  shmem_buffer->packet_size = packet_size;
  shmem_buffer->packet_count = packet_count;

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

int shmem_udp_buffer_open(int gd, int flags, char *name,
                          pirate_channel_t *channel) {
  atomic_uint_fast64_t init_pid;
  int err, fd;
  shmem_buffer_t *shmem_buffer;

  init_pid = 0;
  // on successful shm_open (fd > 0) we must
  // shm_unlink before exiting this function
  fd = shm_open(name, O_RDWR | O_CREAT, 0660);
  if (fd < 0) {
    channel->shmem_buffer = NULL;
    return -1;
  }
  shmem_buffer = shmem_udp_buffer_init(fd, channel);
  if (shmem_buffer == NULL) {
    goto error;
  }
  channel->shmem_buffer = shmem_buffer;
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
  channel->shmem_buffer = NULL;
  shm_unlink(name);
  errno = err;
  return -1;
}

ssize_t shmem_udp_buffer_read(shmem_buffer_t *shmem_buffer, void *buf,
                              size_t count) {
  int spin, was_full;
  size_t packet_size, packet_count;
  uint16_t exp_csum, obs_csum;
  uint32_t reader, writer;
  uint64_t position;
  struct ip_hdr ip_header;
  struct udp_hdr udp_header;
  struct pseudo_ip_hdr pseudo_header;

  if (shmem_buffer == NULL) {
    errno = EBADF;
    return -1;
  }

  packet_size = shmem_buffer->packet_size;
  packet_count = shmem_buffer->packet_count;
  if (count > (packet_size - UDP_HEADER_SIZE)) {
    count = packet_size - UDP_HEADER_SIZE;
  }

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
  atomic_thread_fence(memory_order_acquire);
  memcpy(&ip_header, shmem_buffer->buffer + (reader * packet_size), sizeof(struct ip_hdr));
  memcpy(&udp_header, shmem_buffer->buffer + (reader * packet_size) + sizeof(struct ip_hdr),
         sizeof(struct udp_hdr));
  memcpy(buf, shmem_buffer->buffer + (reader * packet_size) + UDP_HEADER_SIZE,
         count);

  for (;;) {
    uint64_t update = create_position(writer, (reader + 1) % packet_count, 0);
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
  obs_csum = cksum_avx2((void*) &pseudo_header, sizeof(struct pseudo_ip_hdr), 0);
  obs_csum = cksum_avx2((void*) buf, count, ~obs_csum);
  if (exp_csum != obs_csum) {
    errno = EL3HLT;
    return -1;
  }

  return count;
}

ssize_t shmem_udp_buffer_write(shmem_buffer_t *shmem_buffer, const void *buf,
                               size_t size) {
  int spin, was_empty;
  size_t packet_size, packet_count;
  uint16_t csum;
  uint32_t reader, writer;
  uint64_t position;
  struct ip_hdr ip_header;
  struct udp_hdr udp_header;
  struct pseudo_ip_hdr pseudo_header;

  if (shmem_buffer == NULL) {
    errno = EBADF;
    return -1;
  }

  packet_size = shmem_buffer->packet_size;
  packet_count = shmem_buffer->packet_count;
  if (size > (packet_size - UDP_HEADER_SIZE)) {
    size = packet_size - UDP_HEADER_SIZE;
  }

  memset(&ip_header, 0, sizeof(struct ip_hdr));
  ip_header.version = 4;
  ip_header.ihl = 5;
  ip_header.tos = 16; // low delay
  ip_header.len = size + UDP_HEADER_SIZE;
  ip_header.ttl = 1;
  ip_header.proto = 17; // UDP
  ip_header.srcaddr = IPV4(127, 0, 0, 1);
  ip_header.dstaddr = IPV4(127, 0, 0, 1);
  ip_header.csum = cksum_avx2((void*) &ip_header, sizeof(struct ip_hdr), 0);

  udp_header.srcport = 0;
  udp_header.dstport = 0;
  udp_header.len = size + sizeof(struct udp_hdr);
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
  csum = cksum_avx2((void*) buf, size, ~csum);
  udp_header.csum = csum;

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
  memcpy(shmem_buffer->buffer + (writer * packet_size), &ip_header,
         sizeof(struct ip_hdr));
  memcpy(shmem_buffer->buffer + (writer * packet_size) + sizeof(struct ip_hdr),
         &udp_header, sizeof(struct udp_hdr));
  memcpy(shmem_buffer->buffer + (writer * packet_size) + UDP_HEADER_SIZE, buf,
         size);
  atomic_thread_fence(memory_order_release);
  for (;;) {
    uint64_t update = create_position((writer + 1) % packet_count, reader, 1);
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
  return size;
}

int shmem_udp_buffer_close(int flags, shmem_buffer_t *shmem_buffer) {
  size_t buffer_size = sizeof(shmem_buffer_t) +
                       (shmem_buffer->packet_size * shmem_buffer->packet_count);
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
