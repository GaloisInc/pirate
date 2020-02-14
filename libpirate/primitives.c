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
#include <sys/uio.h>
#include <unistd.h>

#include "primitives.h"
#include "ge_eth.h"
#include "mercury.h"
#include "serial.h"
#include "shmem_interface.h"
#include "shmem_udp_interface.h"
#include "tcp_socket.h"
#include "udp_socket.h"
#include "uio.h"
#include "unix_socket.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static pirate_channel_t readers[PIRATE_NUM_CHANNELS] = {
    {0, PIPE, NULL, 0, 0, 0, 0, NULL, 0}};
static pirate_channel_t writers[PIRATE_NUM_CHANNELS] = {
    {0, PIPE, NULL, 0, 0, 0, 0, NULL, 0}};

// gaps descriptors must be opened from smallest to largest
int pirate_open(int gd, int flags) {
  pirate_channel_t *channels;
  int fd, rv, len;
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

  memset(pathname, 0, PIRATE_LEN_NAME);
  switch (channels[gd].channel) {
  case PIPE:
    /* Create a named pipe, if one does not exist */
    snprintf(pathname, PIRATE_LEN_NAME, PIRATE_FILENAME, gd);
    rv = mkfifo(pathname, 0660);
    if (rv == -1) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        return -1;
      }
    }
    break;
  case DEVICE:
    if (channels[gd].pathname == NULL) {
      errno = EINVAL;
      return -1;
    }
    len = strnlen(channels[gd].pathname, PIRATE_LEN_NAME);
    if ((len <= 0) || (len >= PIRATE_LEN_NAME)) {
      errno = EINVAL;
      return -1;
    }
    strncpy(pathname, channels[gd].pathname, PIRATE_LEN_NAME - 1);
    break;
  case UNIX_SOCKET:
    fd = pirate_unix_socket_open(gd, flags, channels);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;
    return gd;
  case TCP_SOCKET:
    if ((flags == O_WRONLY) && (channels[gd].pathname == NULL)) {
      errno = EINVAL;
      return -1;
    }
    fd = pirate_tcp_socket_open(gd, flags, channels);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;
    return gd;
  case UDP_SOCKET:
    if ((flags == O_WRONLY) && (channels[gd].pathname == NULL)) {
      errno = EINVAL;
      return -1;
    }
    fd = pirate_udp_socket_open(gd, flags, channels);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;
    return gd;
  case SHMEM:
    return pirate_shmem_open(gd, flags, channels);
  case SHMEM_UDP:
    return pirate_shmem_udp_open(gd, flags, channels);
  case UIO_DEVICE:
    fd = uio_buffer_open(gd, flags, channels);
    if (fd < 0) {
      return -1;
    }
    channels[gd].fd = fd;
    return gd;
  case SERIAL:
    if (channels[gd].pathname == NULL) {
      return -1;
    }
    return pirate_serial_open(gd, flags, channels);
  case MERCURY:
    return pirate_mercury_open(gd, flags, channels);
  case GE_ETH:
    return pirate_ge_eth_open(gd, flags, channels);
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

  switch (channels[gd].channel) {
  case SHMEM:
    return pirate_shmem_close(gd, channels);
  case SHMEM_UDP:
    return pirate_shmem_udp_close(gd, channels);
  case UIO_DEVICE:
    uio_buffer_close(flags, channels[gd].shmem_buffer);
    channels[gd].shmem_buffer = NULL;
    break;
  case MERCURY:
    return pirate_mercury_close(gd, channels);
  default:
    break;
  }

  fd = channels[gd].fd;
  if (fd <= 0) {
    errno = ENODEV;
    return -1;
  }
  channels[gd].fd = 0;
  rv = close(fd);
  return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
  int i, fd, iovcnt;
  struct iovec iov[PIRATE_IOV_MAX];
  unsigned char *iov_base;
  size_t iov_len;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  switch (readers[gd].channel) {
  case SHMEM:
    return pirate_shmem_read(readers[gd].shmem_buffer, buf, count);
  case SHMEM_UDP:
    return pirate_shmem_udp_read(readers[gd].shmem_buffer, buf, count);
  case UIO_DEVICE:
    return uio_buffer_read(readers[gd].shmem_buffer, buf, count);
  case UDP_SOCKET:
    return pirate_udp_socket_read(gd, readers, buf, count);
  case MERCURY:
    return pirate_mercury_read(gd, readers, buf, count);
  case GE_ETH:
    return pirate_ge_eth_read(gd, readers, buf, count);
  default:
    break;
  }

  fd = readers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  if ((readers[gd].iov_len > 0) && (count > readers[gd].iov_len)) {
    iovcnt = count / readers[gd].iov_len;
    if ((count % readers[gd].iov_len) != 0) {
      iovcnt += 1;
    }
    iovcnt = MIN(iovcnt, PIRATE_IOV_MAX);
    iov_base = buf;
    for (i = 0; i < iovcnt; i++) {
      iov_len = MIN(count, readers[gd].iov_len);
      iov[i].iov_base = iov_base;
      iov[i].iov_len = iov_len;
      iov_base += iov_len;
      count -= iov_len;
    }
    return readv(fd, iov, iovcnt);
  } else {
    return read(fd, buf, count);
  }
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
  int i, fd, iovcnt;
  struct iovec iov[PIRATE_IOV_MAX];
  unsigned char *iov_base;
  size_t iov_len;

  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }

  switch (writers[gd].channel) {
  case SHMEM:
    return pirate_shmem_write(writers[gd].shmem_buffer, buf, count);
  case SHMEM_UDP:
    return pirate_shmem_udp_write(writers[gd].shmem_buffer, buf, count);
  case UIO_DEVICE:
    return uio_buffer_write(writers[gd].shmem_buffer, buf, count);
  case UDP_SOCKET:
    return pirate_udp_socket_write(gd, writers, buf, count);
  case SERIAL:
    return pirate_serial_write(gd, writers, buf, count);
  case MERCURY:
    return pirate_mercury_write(gd, writers, buf, count);
  case GE_ETH:
    return pirate_ge_eth_write(gd, writers, buf, count);
  default:
    break;
  }

  fd = writers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  if ((writers[gd].iov_len > 0) && (count > writers[gd].iov_len)) {
    iovcnt = count / writers[gd].iov_len;
    if ((count % writers[gd].iov_len) != 0) {
      iovcnt += 1;
    }
    iovcnt = MIN(iovcnt, PIRATE_IOV_MAX);
    iov_base = (void*) buf;
    for (i = 0; i < iovcnt; i++) {
      iov_len = MIN(count, writers[gd].iov_len);
      iov[i].iov_base = iov_base;
      iov[i].iov_len = iov_len;
      iov_base += iov_len;
      count -= iov_len;
    }
    return writev(fd, iov, iovcnt);
  } else {
    return write(fd, buf, count);
  }
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

int pirate_getsockopt(int gd, int flags, int level, int optname, void *optval,
                      socklen_t *optlen) {
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
  return getsockopt(fd, level, optname, optval, optlen);
}


int pirate_setsockopt(int gd, int flags, int level, int optname,
                      const void *optval, socklen_t optlen) {
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
  return setsockopt(fd, level, optname, optval, optlen);
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

int pirate_set_pathname(int gd, const char *pathname) {
  int len;
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
    len = strnlen(pathname, PIRATE_LEN_NAME);
    if ((len <= 0) || (len >= PIRATE_LEN_NAME)) {
      errno = EINVAL;
      return - 1;
    }
    if (readers[gd].pathname == NULL) {
      readers[gd].pathname = calloc(PIRATE_LEN_NAME, sizeof(char));
    }
    if (writers[gd].pathname == NULL) {
      writers[gd].pathname = calloc(PIRATE_LEN_NAME, sizeof(char));
    }
    strncpy(readers[gd].pathname, pathname, PIRATE_LEN_NAME - 1);
    strncpy(writers[gd].pathname, pathname, PIRATE_LEN_NAME - 1);
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

int pirate_set_port_number(int gd, int port) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].port_number = port;
  writers[gd].port_number = port;
  return 0;
}

int pirate_get_port_number(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  return readers[gd].port_number;
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

int pirate_set_packet_size(int gd, size_t packet_size) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].packet_size = packet_size;
  writers[gd].packet_size = packet_size;
  return 0;
}

size_t pirate_get_packet_size(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  return readers[gd].packet_size;
}

int pirate_set_packet_count(int gd, size_t packet_count) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].packet_count = packet_count;
  writers[gd].packet_count = packet_count;
  return 0;
}

size_t pirate_get_packet_count(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  return readers[gd].packet_count;
}

int pirate_set_iov_length(int gd, size_t iov_len) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  readers[gd].iov_len = iov_len;
  writers[gd].iov_len = iov_len;
  return 0;
}

ssize_t pirate_get_iov_length(int gd) {
  if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
    errno = EBADF;
    return -1;
  }
  return readers[gd].iov_len;
}
