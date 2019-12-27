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

static int tcp_socket_reader_open(int gd, pirate_channel_t *channels) {
  int fd, rv, err, enable, buffer_size, server_fd;
  struct sockaddr_in addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    return server_fd;
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PIRATE_PORT_NUMBER + gd);

  enable = 1;
  rv = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  if (rv < 0) {
    err = errno;
    close(server_fd);
    errno = err;
    return rv;
  }

  buffer_size = channels[gd].buffer_size;
  if (buffer_size > 0) {
    rv = setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size,
                    sizeof(buffer_size));
    if (rv < 0) {
      err = errno;
      close(server_fd);
      errno = err;
      return rv;
    }
  }

  rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (rv < 0) {
    err = errno;
    close(server_fd);
    errno = err;
    return rv;
  }

  rv = listen(server_fd, 0);
  if (rv < 0) {
    err = errno;
    close(server_fd);
    errno = err;
    return rv;
  }

  fd = accept(server_fd, NULL, NULL);

  if (fd < 0) {
    err = errno;
    close(server_fd);
    errno = err;
    return fd;
  }

  close(server_fd);
  return fd;
}

static int tcp_socket_writer_open(int gd, pirate_channel_t *channels) {
  int fd, rv, buffer_size, err;
  struct sockaddr_in addr;
  struct timespec req;

  fd = socket(AF_INET, SOCK_STREAM, 0);
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

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(channels[gd].pathname);
  addr.sin_port = htons(PIRATE_PORT_NUMBER + gd);

  for (;;) {
    rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) {
      if ((errno == ENOENT) || (errno == ECONNREFUSED)) {
        req.tv_sec = 0;
        req.tv_nsec = 1e8;
        rv = nanosleep(&req, NULL);
        if (rv == 0) {
          continue;
        }
      }
      err = errno;
      close(fd);
      errno = err;
      return rv;
    }
    return fd;
  }
}

int pirate_tcp_socket_open(int gd, int flags, pirate_channel_t *channels) {
  if (flags == O_RDONLY) {
    return tcp_socket_reader_open(gd, channels);
  } else {
    return tcp_socket_writer_open(gd, channels);
  }
}
