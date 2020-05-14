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
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include "libpirate.h"
#include "pirate_common.h"

ssize_t pirate_fd_read(int fd, void *buf, size_t count) {
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    return read(fd, buf, count);
}

ssize_t pirate_fd_write(int fd, const void *buf, size_t count) {
    if (fd < 0) {
        errno = EBADF;
        return -1;
    }

    return write(fd, buf, count);
}

int pirate_parse_key_value(char **key, char **val, char *ptr, char **saveptr) {
    *key = strtok_r(ptr, KV_DELIM, saveptr);
    if (*key == NULL) {
        errno = EINVAL;
        return -1;
    }
    *val = strtok_r(NULL, KV_DELIM, saveptr);
    if (*val == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (pirate_parse_is_common_key(*key)) {
        return 0;
    }
    return 1;
}
