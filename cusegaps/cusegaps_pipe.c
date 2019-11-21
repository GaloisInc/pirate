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
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fuse.h>
#include <fuse/cuse_lowlevel.h>

#include "fioc.h"

#define PIRATE_FILENAME "/tmp/gaps.channel.%s"

static char dev_name[128];
static char reader_buf[PIPE_BUF];
static int reader_fd, writer_fd;

static const char *usage =
    "usage: cusegaps_pipe [options]\n"
    "\n"
    "options:\n"
    "    --help|-h              print this help message\n"
    "    --name|-n NAME         device name (mandatory)\n"
    "\n";

static void cusegaps_open(fuse_req_t req, struct fuse_file_info *fi) {
  char pathname[128];
  int rv, flags, *fd_p;

  flags = fi->flags & 0x3;
  switch (flags) {
  case O_RDONLY:
    fd_p = &reader_fd;
    break;
  case O_WRONLY:
    fd_p = &writer_fd;
    break;
  default:
    fuse_reply_err(req, EINVAL);
    return;
  }

  if (*fd_p != 0) {
    fuse_reply_err(req, EBUSY);
    return;
  }

  snprintf(pathname, sizeof(pathname) - 1, PIRATE_FILENAME, dev_name);
  rv = mkfifo(pathname, 0660);
  if ((rv == -1) && (errno != EEXIST)) {
    fuse_reply_err(req, errno);
    return;
  }

  *fd_p = open(pathname, flags);
  if (*fd_p < 0) {
    *fd_p = 0;
    fuse_reply_err(req, errno);
    return;
  }

  fuse_reply_open(req, fi);
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
  const struct fuse_ctx *fuse_ctx;

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
  (void)in_buf;
  (void)in_bufsz;
  (void)out_bufsz;

  int rv, fd, data, fi_flags;

  if (flags & FUSE_IOCTL_COMPAT) {
    fuse_reply_err(req, ENOSYS);
    return;
  }

  fi_flags = fi->flags & 0x3;

  printf("Received ioctl command %d\n", cmd);
  switch (cmd) {
  case F_SETPIPE_SZ:
    if (fi_flags == O_RDONLY) {
      fd = reader_fd;
    } else if (fi_flags == O_WRONLY) {
      fd = writer_fd;
    } else {
      fuse_reply_err(req, EINVAL);
      return;
    }
    data = (uintptr_t)arg;
    rv = fcntl(fd, F_SETPIPE_SZ, data);
    if (rv < 0) {
      fuse_reply_err(req, errno);
    } else {
      fuse_reply_ioctl(req, 0, NULL, 0);
    }
    break;
  default:
    fuse_reply_err(req, EINVAL);
  }
}

struct cusegaps_param {
  char *dev_name;
  int is_help;
};

#define CUSEXMP_OPT(t, p)                                                      \
  { t, offsetof(struct cusegaps_param, p), 1 }

static const struct fuse_opt cusegaps_opts[] = {
    CUSEXMP_OPT("-n %s", dev_name), CUSEXMP_OPT("--name %s", dev_name),
    FUSE_OPT_KEY("-h", 0), FUSE_OPT_KEY("--help", 0), FUSE_OPT_END};

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
  struct cusegaps_param param = {NULL, 0};
  char cuse_dev_name[128] = "DEVNAME=";
  const char *dev_info_argv[] = {cuse_dev_name};
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
    strncat(cuse_dev_name, param.dev_name, sizeof(cuse_dev_name) - 9);
    strncpy(dev_name, param.dev_name, sizeof(dev_name));
  }

  memset(&ci, 0, sizeof(ci));
  ci.dev_major = 0;
  ci.dev_minor = 0;
  ci.dev_info_argc = 1;
  ci.dev_info_argv = dev_info_argv;
  ci.flags = CUSE_UNRESTRICTED_IOCTL;
  return cuse_lowlevel_main(args.argc, args.argv, &ci, &cusegaps_clop, NULL);
}
