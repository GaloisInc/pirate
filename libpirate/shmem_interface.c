#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "primitives.h"

int pirate_shmem_open(int gd, int flags, pirate_channel_t *channels) {
#ifdef PIRATE_SHMEM_FEATURE
  char pathname[PIRATE_LEN_NAME];
  snprintf(pathname, sizeof(pathname) - 1, PIRATE_SHM_NAME, gd);
  return shmem_buffer_open(gd, flags, channels[gd].shmem_size, pathname,
                           &channels[gd].shmem_buffer);
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

ssize_t pirate_shmem_read(int gd, void *buf, size_t count,
                          pirate_channel_t *readers) {
#ifdef PIRATE_SHMEM_FEATURE
  if (readers[gd].shmem_buffer == NULL) {
    errno = EBADF;
    return -1;
  } else {
    return shmem_buffer_read(readers[gd].shmem_buffer, buf, count);
  }
#else
  (void)gd;
  (void)buf;
  (void)count;
  (void)readers;
  errno = ENXIO;
  return -1;
#endif
}

ssize_t pirate_shmem_write(int gd, const void *buf, size_t count,
                           pirate_channel_t *writers) {
#ifdef PIRATE_SHMEM_FEATURE
  if (writers[gd].shmem_buffer == NULL) {
    errno = EBADF;
    return -1;
  } else {
    return shmem_buffer_write(writers[gd].shmem_buffer, buf, count);
  }
#else
  (void)gd;
  (void)buf;
  (void)count;
  (void)writers;
  errno = ENXIO;
  return -1;
#endif
}
