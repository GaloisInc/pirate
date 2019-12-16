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
#include <stdio.h>
#include <stdlib.h>

#include "primitives.h"
#include "shmem.h"

int pirate_shmem_open(int gd, int flags, pirate_channel_t *channels) {
#ifdef PIRATE_SHMEM_FEATURE
  char pathname[PIRATE_LEN_NAME];
  memset(pathname, 0, PIRATE_LEN_NAME);
  snprintf(pathname, PIRATE_LEN_NAME, PIRATE_SHM_NAME, gd);
  return shmem_buffer_open(gd, flags, pathname, &channels[gd]);
#else
  (void)gd;
  (void)flags;
  (void)channels;
  errno = ENXIO;
  return -1;
#endif
}

int pirate_shmem_close(int gd, pirate_channel_t *channels) {
#ifdef PIRATE_SHMEM_FEATURE
  int rv;
  if (channels[gd].shmem_buffer != NULL) {
    rv = shmem_buffer_close(gd, channels[gd].shmem_buffer);
  } else {
    errno = ENODEV;
    return -1;
  }
  channels[gd].shmem_buffer = NULL;
  return rv;
#else
  (void)gd;
  (void)channels;
  errno = ENXIO;
  return -1;
#endif
}

ssize_t pirate_shmem_read(shmem_buffer_t *shmem_buffer, void *buf,
                          size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
  return shmem_buffer_read(shmem_buffer, buf, count);
#else
  (void)shmem_buffer;
  (void)buf;
  (void)count;
  errno = ENXIO;
  return -1;
#endif
}

ssize_t pirate_shmem_write(shmem_buffer_t *shmem_buffer, const void *buf,
                           size_t count) {
#ifdef PIRATE_SHMEM_FEATURE
  return shmem_buffer_write(shmem_buffer, buf, count);
#else
  (void)shmem_buffer;
  (void)buf;
  (void)count;
  errno = ENXIO;
  return -1;
#endif
}
