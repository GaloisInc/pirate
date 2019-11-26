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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "primitives.h"
#include "shmem_interface.h"
#include "unix_socket.h"

static pirate_channel_t readers[PIRATE_NUM_CHANNELS] = {
    {0, PIPE, NULL, 0, NULL}};
static pirate_channel_t writers[PIRATE_NUM_CHANNELS] = {
    {0, PIPE, NULL, 0, NULL}};

// gaps descriptors must be opened from smallest to largest
int pirate_open(int gd, int flags) {
  pirate_channel_t *channels;
  int fd, rv;
  char pathname[PIRATE_LEN_NAME];

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  fd = channels[gd].fd;
  if (fd > 0) {
    return gd;
  }

  switch (channels[gd].channel) {
  case PIPE:
    /* Create a named pipe, if one does not exist */
    snprintf(pathname, sizeof(pathname) - 1, PIRATE_FILENAME, gd);
    rv = mkfifo(pathname, 0660);
    if ((rv == -1) && (errno != EEXIST)) {
      return -1;
    }
    break;
  case DEVICE:
    if (channels[gd].pathname == NULL) {
      errno = EINVAL;
      return -1;
    }
    if (strnlen(channels[gd].pathname, PIRATE_LEN_NAME) == 0) {
      errno = EINVAL;
      return -1;
    }
    strncpy(pathname, channels[gd].pathname, PIRATE_LEN_NAME);
    break;
  case UNIX_SOCKET:
    fd = pirate_unix_socket_open(gd, flags, channels[gd].buffer_size);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;
    return gd;
  case SHMEM:
    return pirate_shmem_open(gd, flags, channels);
  case INVALID:
    errno = EINVAL;
    return -1;
  }
  fd = open(pathname, flags);
  if (fd < 0) {
    return -1;
  }

  /* Success */
  channels[gd].fd = fd;
  return gd;
}

int pirate_close(int gd, int flags) {
  pirate_channel_t *channels;
  int rv, fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  if (channels[gd].channel == SHMEM) {
    return pirate_shmem_close(gd, channels);
  } else {
    fd = channels[gd].fd;
    if (fd <= 0) {
      errno = ENODEV;
      return -1;
    }
    channels[gd].fd = 0;
    rv = close(fd);
  }
  return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if (readers[gd].channel == SHMEM) {
    return pirate_shmem_read(gd, buf, count, readers);
  }

  fd = readers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  return read(fd, buf, count);
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if (writers[gd].channel == SHMEM) {
    return pirate_shmem_write(gd, buf, count, writers);
  }

  fd = writers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  return write(fd, buf, count);
}

int pirate_fcntl0(int gd, int flags, int cmd) {
  pirate_channel_t *channels;
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  fd = channels[gd].fd;
  if (fd <= 0) {
    errno = ENODEV;
    return -1;
  }

  return fcntl(fd, cmd);
}

int pirate_fcntl1_int(int gd, int flags, int cmd, int arg) {
  pirate_channel_t *channels;
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  fd = channels[gd].fd;
  if (fd <= 0) {
    errno = ENODEV;
    return -1;
  }

  return fcntl(fd, cmd, arg);
}

int pirate_ioctl0(int gd, int flags, long cmd) {
  pirate_channel_t *channels;
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  fd = channels[gd].fd;
  if (fd <= 0) {
    errno = ENODEV;
    return -1;
  }

  return ioctl(fd, cmd);
}

int pirate_ioctl1_int(int gd, int flags, long cmd, int arg) {
  pirate_channel_t *channels;
  int fd;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
    errno = EINVAL;
    return -1;
  }

  if (flags == O_RDONLY) {
    channels = readers;
  } else {
    channels = writers;
  }

  fd = channels[gd].fd;
  if (fd <= 0) {
    errno = ENODEV;
    return -1;
  }

  return ioctl(fd, cmd, arg);
}

int pirate_set_channel_type(int gd, channel_t channel_type) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].channel = channel_type;
  writers[gd].channel = channel_type;
  return 0;
}

channel_t pirate_get_channel_type(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    return INVALID;
  }
  return readers[gd].channel;
}

int pirate_set_pathname(int gd, char *pathname) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  if (pathname == NULL) {
    if (readers[gd].pathname != NULL) {
      free(readers[gd].pathname);
      readers[gd].pathname = NULL;
    }
    if (writers[gd].pathname != NULL) {
      free(writers[gd].pathname);
      writers[gd].pathname = NULL;
    }
  } else {
    if (readers[gd].pathname == NULL) {
      readers[gd].pathname = calloc(PIRATE_LEN_NAME, sizeof(char));
    }
    if (writers[gd].pathname == NULL) {
      writers[gd].pathname = calloc(PIRATE_LEN_NAME, sizeof(char));
    }
    strncpy(readers[gd].pathname, pathname, PIRATE_LEN_NAME);
    strncpy(writers[gd].pathname, pathname, PIRATE_LEN_NAME);
  }
  return 0;
}

int pirate_get_pathname(int gd, char *pathname) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  if (readers[gd].pathname != NULL) {
    strncpy(pathname, readers[gd].pathname, PIRATE_LEN_NAME);
  }
  return 0;
}

int pirate_set_buffer_size(int gd, int buffer_size) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].buffer_size = buffer_size;
  writers[gd].buffer_size = buffer_size;
  return 0;
}

int pirate_get_buffer_size(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  return readers[gd].buffer_size;
}
