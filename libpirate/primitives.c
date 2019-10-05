#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "primitives.h"

typedef struct {
    int fd;             // pipe file descriptor
} pirate_channel_t;

static pirate_channel_t readers[PIRATE_NUM_CHANNELS] = {0};
static pirate_channel_t writers[PIRATE_NUM_CHANNELS] = {0};

int pirate_open(int gd, int flags) {
    pirate_channel_t* channels;
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
    if (fd != 0) {
        return gd;
    }

    /* Create a named pipe, if one does not exist */
    snprintf(pathname, sizeof(pathname) - 1, PIRATE_FILENAME, gd);
    rv = mkfifo(pathname, 0660);
    if ((rv == -1) && (errno != EEXIST)) {
        return -1;
    }

    fd = open(pathname, flags);
    if (fd < 0) {
        return -1;
    }

    /* Success */
    channels[gd] = (pirate_channel_t){fd};
    return gd;
}

int pirate_close(int gd) {
    int fd1, fd2;
    int rv1 = 0, rv2 = 0;

    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = EBADF;
        return -1;
    }

    fd1 = readers[gd].fd;
    fd2 = writers[gd].fd;

    if ((fd1 == 0) && (fd2 == 0)) {
        errno = ENODEV;
        return -1;
    }

    if (fd1 > 0) {
        rv1 = close(fd1);
    }

    if (fd2 > 0) {
        rv2 = close(fd2);
    }

    if (rv1 != 0) {
        return rv1;
    }

    if (rv2 != 0) {
        return rv2;
    }

    return 0;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    int fd;

    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = EBADF;
        return -1;
    }

    fd = readers[gd].fd;
    if (fd == 0) {
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

    fd = writers[gd].fd;
    if (fd == 0) {
        errno = EBADF;
        return -1;
    }

    return write(fd, buf, count);
}
