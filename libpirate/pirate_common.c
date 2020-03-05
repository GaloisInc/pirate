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
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include "libpirate.h"
#include "pirate_common.h"

static int create_iov(void *buf, size_t count, size_t iov_len,
    struct iovec *iov) {
    int iov_count = count / iov_len;
    unsigned char *iov_base = buf;
    if (count > iov_len * iov_count) {
        ++iov_count;
    }

    iov_count = MIN(iov_count, PIRATE_IOV_MAX);

    for (int i = 0; i < iov_count; i++) {
        iov[i].iov_base = iov_base;
        iov[i].iov_len = MIN(count, iov_len);
        iov_base += iov[i].iov_len;
        count -= iov[i].iov_len;
    }

    return iov_count;
}

ssize_t pirate_fd_read(int fd, void *buf, size_t count, size_t iov_len) {
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if ((iov_len > 0) && (count > iov_len)) {
        struct iovec iov[PIRATE_IOV_MAX];
        int iov_count = create_iov(buf, count, iov_len, iov);
        return readv(fd, iov, iov_count);
    }

    return read(fd, buf, count);
}

ssize_t pirate_fd_write(int fd, const void *buf, size_t count, size_t iov_len) {
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    if ((iov_len > 0) && (count > iov_len)) {
        struct iovec iov[PIRATE_IOV_MAX];
        int iov_count = create_iov((void *)buf, count, iov_len, iov);
        return writev(fd, iov, iov_count);
    }

    return write(fd, buf, count);
}
