#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "primitives.h"

static int udp_socket_reader_open(int gd, pirate_channel_t *channels) {
  int fd, rv, err, enable, buffer_size;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return fd;
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PIRATE_PORT_NUMBER + gd);

  enable = 1;
  rv = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (rv < 0) {
    err = errno;
    close(fd);
    errno = err;
    return rv;
  }

  buffer_size = channels[gd].buffer_size;
  if (buffer_size > 0) {
    rv = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size,
                    sizeof(buffer_size));
    if (rv < 0) {
      err = errno;
      close(fd);
      errno = err;
      return rv;
    }
  }

  rv = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (rv < 0) {
    err = errno;
    close(fd);
    errno = err;
    return rv;
  }

  return fd;
}

static int udp_socket_writer_open(int gd, pirate_channel_t *channels) {
  int fd, rv, buffer_size, err;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return fd;
  }

  buffer_size = channels[gd].buffer_size;
  if (buffer_size > 0) {
    rv = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size,
                    sizeof(buffer_size));
    if (rv < 0) {
      err = errno;
      close(fd);
      errno = err;
      return rv;
    }
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(channels[gd].pathname);
  addr.sin_port = htons(PIRATE_PORT_NUMBER + gd);
  rv = connect(fd, (const struct sockaddr*) &addr, sizeof(addr));

  if (rv < 0) {
    err = errno;
    close(fd);
    errno = err;
    return rv;
  }

  return fd;
}

int pirate_udp_socket_open(int gd, int flags, pirate_channel_t *channels) {
  if (flags == O_RDONLY) {
    return udp_socket_reader_open(gd, channels);
  } else {
    return udp_socket_writer_open(gd, channels);
  }
}

ssize_t pirate_udp_socket_read(int gd, pirate_channel_t *readers, void *buf,
                               size_t count) {
  int fd;

  fd = readers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  return recv(fd, buf, count, 0);
}

ssize_t pirate_udp_socket_write(int gd, pirate_channel_t *writers,
                                const void *buf, size_t count) {
  int fd;

  fd = writers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  return send(fd, buf, count, 0);
}
