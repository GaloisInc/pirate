#ifndef __SHMEM_INTERFACE_H
#define __SHMEM_INTERFACE_H

#include "primitives.h"

int pirate_shmem_open(int gd, int flags, pirate_channel_t *channels);
int pirate_shmem_close(int gd, pirate_channel_t *channels);
ssize_t pirate_shmem_read(int gd, void *buf, size_t count,
                          pirate_channel_t *readers);
ssize_t pirate_shmem_write(int gd, const void *buf, size_t count,
                           pirate_channel_t *writers);
#endif