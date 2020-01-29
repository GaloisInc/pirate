#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

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

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static int udp_socket_reader_open(int gd, pirate_channel_t *channels) {
  int fd, rv, err, enable, port, buffer_size;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return fd;
  }

  port = channels[gd].port_number;
  if (port <= 0) {
    port = PIRATE_PORT_NUMBER + gd;
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

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
  int fd, rv, port, buffer_size, err;
  struct sockaddr_in addr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    return fd;
  }

  port = channels[gd].port_number;
  if (port <= 0) {
    port = PIRATE_PORT_NUMBER + gd;
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
  addr.sin_port = htons(port);
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
  int i, rv, fd, vlen;
  struct mmsghdr msgvec[PIRATE_IOV_MAX];
  struct iovec iov[PIRATE_IOV_MAX];
  unsigned char *iov_base;
  size_t iov_len;
  ssize_t numbytes = 0;

  fd = readers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }


  if ((readers[gd].iov_len > 0) && (count > readers[gd].iov_len)) {
    vlen = count / readers[gd].iov_len;
    if ((count % readers[gd].iov_len) != 0) {
      vlen += 1;
    }
    vlen = MIN(vlen, PIRATE_IOV_MAX);
    iov_base = buf;
    memset(msgvec, 0, sizeof(msgvec));
    for (i = 0; i < vlen; i++) {
      iov_len = MIN(count, readers[gd].iov_len);
      iov[i].iov_base = iov_base;
      iov[i].iov_len = iov_len;
      msgvec[i].msg_hdr.msg_name = NULL;
      msgvec[i].msg_hdr.msg_namelen = 0;
      msgvec[i].msg_hdr.msg_iov = &iov[i];
      msgvec[i].msg_hdr.msg_iovlen = 1;
      iov_base += iov_len;
      count -= iov_len;
    }
    rv = recvmmsg(fd, msgvec, vlen, 0, NULL);
    if (rv < 0) {
      return rv;
    }
    for (i = 0; i < rv; i++) {
      numbytes += iov[i].iov_len;
    }
    return numbytes;
  } else {
    return recv(fd, buf, count, 0);
  }
}

ssize_t pirate_udp_socket_write(int gd, pirate_channel_t *writers,
                                const void *buf, size_t count) {
  int i, rv, fd, vlen;
  struct mmsghdr msgvec[PIRATE_IOV_MAX];
  struct iovec iov[PIRATE_IOV_MAX];
  unsigned char *iov_base;
  size_t iov_len;
  ssize_t numbytes = 0;

  fd = writers[gd].fd;
  if (fd <= 0) {
    errno = EBADF;
    return -1;
  }

  if ((writers[gd].iov_len > 0) && (count > writers[gd].iov_len)) {
    vlen = count / writers[gd].iov_len;
    if ((count % writers[gd].iov_len) != 0) {
      vlen += 1;
    }
    vlen = MIN(vlen, PIRATE_IOV_MAX);
    iov_base = (unsigned char*) buf;
    memset(msgvec, 0, sizeof(msgvec));
    for (i = 0; i < vlen; i++) {
      iov_len = MIN(count, writers[gd].iov_len);
      iov[i].iov_base = iov_base;
      iov[i].iov_len = iov_len;
      msgvec[i].msg_hdr.msg_name = NULL;
      msgvec[i].msg_hdr.msg_namelen = 0;
      msgvec[i].msg_hdr.msg_iov = &iov[i];
      msgvec[i].msg_hdr.msg_iovlen = 1;
      iov_base += iov_len;
      count -= iov_len;
    }
    rv = sendmmsg(fd, msgvec, vlen, 0);
    if (rv < 0) {
      return rv;
    }
    for (i = 0; i < rv; i++) {
      numbytes += iov[i].iov_len;
    }
    return numbytes;
  } else {
    return send(fd, buf, count, 0);
  }
}
