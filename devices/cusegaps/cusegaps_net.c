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

#define FUSE_USE_VERSION 31
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fuse.h>
#include <fuse/cuse_lowlevel.h>

#include "fioc.h"

static char reader_buf[PIPE_BUF];
static int reader_fd, writer_fd;
static int port;
static struct sockaddr_in serv_addr;

static const char *usage =
    "usage: cusegaps_net [options]\n"
    "\n"
    "options:\n"
    "    --help|-h              print this help message\n"
    "    --name|-n NAME         device name (mandatory)\n"
    "    --port|-p PORT         port number (mandatory)\n"
    "    --address|-a ADDRESS   reader address (writer mandatory)\n"
    "\n";

static void reader_open(fuse_req_t req, struct fuse_file_info *fi) {
  int err, server_fd, opt = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  struct linger socket_reset;

  socket_reset.l_onoff = 1;
  socket_reset.l_linger = 0;
  if (port <= 0) {
    fuse_reply_err(req, EINVAL);
    return;
  }
  if (reader_fd != 0) {
    fuse_reply_err(req, EBUSY);
    return;
  }
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fuse_reply_err(req, errno);
    return;
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt))) {
    err = errno;
    close(server_fd);
    fuse_reply_err(req, err);
    return;
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))) {
    err = errno;
    close(server_fd);
    fuse_reply_err(req, err);
    return;
  }
  if (listen(server_fd, 1)) {
    err = errno;
    close(server_fd);
    fuse_reply_err(req, err);
    return;
  }
  reader_fd =
      accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  err = errno;
  close(server_fd);
  if (reader_fd < 0) {
    reader_fd = 0;
    fuse_reply_err(req, err);
    return;
  }
  // write should return EPIPE error when write fd is
  // connected to a pipe or socket whose reading end
  // is closed.
  //
  // Send a TCP reset (RST) on the read fd to
  // generate the EPIPE error on the write fd.
  if (setsockopt(reader_fd, SOL_SOCKET, SO_LINGER, &socket_reset,
                 sizeof socket_reset)) {
    err = errno;
    close(reader_fd);
    reader_fd = 0;
    fuse_reply_err(req, err);
  }
  fuse_reply_open(req, fi);
}

static void writer_open(fuse_req_t req, struct fuse_file_info *fi) {
  int err;
  struct timespec tim;

  tim.tv_sec = 0;
  tim.tv_nsec = 100000000L;
  if (serv_addr.sin_family <= 0) {
    fuse_reply_err(req, EINVAL);
    return;
  }
  if (serv_addr.sin_port <= 0) {
    fuse_reply_err(req, EINVAL);
    return;
  }
  if (serv_addr.sin_addr.s_addr == 0) {
    fuse_reply_err(req, EINVAL);
    return;
  }
  if (writer_fd != 0) {
    fuse_reply_err(req, EBUSY);
    return;
  }
  writer_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (writer_fd < 0) {
    writer_fd = 0;
    fuse_reply_err(req, errno);
    return;
  }
  while (connect(writer_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    if (errno == ECONNREFUSED) {
      if (!nanosleep(&tim, NULL)) {
        continue;
      }
    }
    err = errno;
    close(writer_fd);
    writer_fd = 0;
    fuse_reply_err(req, err);
    return;
  }
  fuse_reply_open(req, fi);
}

static void cusegaps_open(fuse_req_t req, struct fuse_file_info *fi) {
  int flags = fi->flags & 0x3;

  switch (flags) {
  case O_RDONLY:
    reader_open(req, fi);
    break;
  case O_WRONLY:
    writer_open(req, fi);
    break;
  default:
    fuse_reply_err(req, EINVAL);
  }
}

static void cusegaps_release(fuse_req_t req, struct fuse_file_info *fi) {
  int rv, fd, *fd_p;
  int flags = fi->flags & 0x3;

  switch (flags) {
  case O_RDONLY:
    fd_p = &reader_fd;
    fd = reader_fd;
    break;
  case O_WRONLY:
    fd_p = &writer_fd;
    fd = writer_fd;
    break;
  default:
    // this should never happen
    fuse_reply_err(req, EINVAL);
    return;
  }

  if (fd <= 0) {
    fuse_reply_err(req, EBADF);
    return;
  }
  rv = close(fd);
  *fd_p = 0;

  if (rv) {
    fuse_reply_err(req, errno);
  } else {
    fuse_reply_err(req, 0);
  }
}

static void cusegaps_read(fuse_req_t req, size_t size, off_t off,
                          struct fuse_file_info *fi) {
  (void)off;
  (void)fi;
  int nbytes;

  if (size > sizeof(reader_buf)) {
    size = sizeof(reader_buf);
  }
  nbytes = read(reader_fd, reader_buf, size);
  if (nbytes < 0) {
    fuse_reply_err(req, errno);
  } else {
    fuse_reply_buf(req, reader_buf, nbytes);
  }
}

static void cusegaps_write(fuse_req_t req, const char *buf, size_t size,
                           off_t off, struct fuse_file_info *fi) {
  (void)off;
  (void)fi;
  int err, nbytes;
  const struct fuse_ctx* fuse_ctx;

  nbytes = write(writer_fd, buf, size);
  if (nbytes < 0) {
    err = errno;
    // If all file descriptors referring to the read end of a pipe
    // have been closed, then a write(2) will cause a SIGPIPE
    // signal to be generated for the calling process.
    if (err == EPIPE) {
      fuse_ctx = fuse_req_ctx(req);
      if ((fuse_ctx != NULL) && (fuse_ctx->pid > 0)) {
        kill(fuse_ctx->pid, SIGPIPE);
      }
    }
    fuse_reply_err(req, err);
  } else {
    fuse_reply_write(req, nbytes);
  }
}

static void cusegaps_ioctl(fuse_req_t req, int cmd, void *arg,
                           struct fuse_file_info *fi, unsigned flags,
                           const void *in_buf, size_t in_bufsz,
                           size_t out_bufsz) {
  (void)arg;
  (void)fi;
  (void)in_buf;
  (void)in_bufsz;
  (void)out_bufsz;

  if (flags & FUSE_IOCTL_COMPAT) {
    fuse_reply_err(req, ENOSYS);
    return;
  }

  printf("Received ioctl command %d\n", cmd);

  fuse_reply_err(req, EINVAL);
}

struct cusegaps_param {
  char *dev_name;
  int port;
  char *address;
  int is_help;
};

#define CUSEXMP_OPT(t, p)                                                      \
  { t, offsetof(struct cusegaps_param, p), 1 }

static const struct fuse_opt cusegaps_opts[] = {
    CUSEXMP_OPT("-n %s", dev_name),
    CUSEXMP_OPT("--name %s", dev_name),
    CUSEXMP_OPT("-p %u", port),
    CUSEXMP_OPT("--port %u", port),
    CUSEXMP_OPT("-a %s", address),
    CUSEXMP_OPT("--address %s", address),
    FUSE_OPT_KEY("-h", 0),
    FUSE_OPT_KEY("--help", 0),
    FUSE_OPT_END};

static int cusegaps_process_arg(void *data, const char *arg, int key,
                                struct fuse_args *outargs) {
  struct cusegaps_param *param = data;

  (void)outargs;
  (void)arg;

  switch (key) {
  case 0:
    param->is_help = 1;
    fprintf(stderr, "%s", usage);
    return fuse_opt_add_arg(outargs, "-ho");
  default:
    return 1;
  }
}

static const struct cuse_lowlevel_ops cusegaps_clop = {
    .open = cusegaps_open,
    .read = cusegaps_read,
    .write = cusegaps_write,
    .ioctl = cusegaps_ioctl,
    .release = cusegaps_release,
};

int main(int argc, char **argv) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  struct cusegaps_param param = {NULL, 0, NULL, 0};
  char dev_name[128] = "DEVNAME=";
  const char *dev_info_argv[] = {dev_name};
  struct cuse_info ci;

  if (fuse_opt_parse(&args, &param, cusegaps_opts, cusegaps_process_arg)) {
    printf("failed to parse option\n");
    return 1;
  }

  if (!param.is_help) {
    if (!param.dev_name) {
      fprintf(stderr, "Error: device name missing\n");
      return 1;
    }
    if (param.port == 0) {
      fprintf(stderr, "Error: port number missing\n");
      return 1;
    }
    if (param.port < 0) {
      fprintf(stderr, "Error: port number must be positive integer\n");
      return 1;
    }
    if (param.address) {
      if (inet_pton(AF_INET, param.address, &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: invalid address %s\n", param.address);
      }
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(param.port);
    }
    strncat(dev_name, param.dev_name, sizeof(dev_name) - 9);
  }

  memset(&ci, 0, sizeof(ci));
  ci.dev_major = 0;
  ci.dev_minor = 0;
  ci.dev_info_argc = 1;
  ci.dev_info_argv = dev_info_argv;
  ci.flags = CUSE_UNRESTRICTED_IOCTL;
  port = param.port;
  return cuse_lowlevel_main(args.argc, args.argv, &ci, &cusegaps_clop, NULL);
}
