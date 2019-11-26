#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "primitives.h"

static int unix_socket_reader_open(int gd, int buffer_size) {
  int fd, rv, err, server_fd;
  struct sockaddr_un addr;
  char pathname[PIRATE_LEN_NAME];

  server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_fd < 0) {
    return server_fd;
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  snprintf(pathname, sizeof(pathname) - 1, PIRATE_DOMAIN_FILENAME, gd);
  strncpy(addr.sun_path, pathname, sizeof(addr.sun_path) - 1);

  if (buffer_size > 0) {
    rv = setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size,
                    sizeof(buffer_size));
    if (rv < 0) {
      err = errno;
      close(server_fd);
      errno = err;
      return rv;
    }
  }

  unlink(pathname);
  rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
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

static int unix_socket_writer_open(int gd, int buffer_size) {
  int fd, rv, err;
  struct sockaddr_un addr;
  struct timespec req;

  char pathname[PIRATE_LEN_NAME];

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    return fd;
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  snprintf(pathname, sizeof(pathname) - 1, PIRATE_DOMAIN_FILENAME, gd);
  strncpy(addr.sun_path, pathname, sizeof(addr.sun_path) - 1);

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

int pirate_unix_socket_open(int gd, int flags, int buffer_size) {
  if (flags == O_RDONLY) {
    return unix_socket_reader_open(gd, buffer_size);
  } else {
    return unix_socket_writer_open(gd, buffer_size);
  }
}
