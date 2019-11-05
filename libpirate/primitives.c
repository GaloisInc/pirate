#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "primitives.h"

typedef struct {
    int fd;                         // file descriptor
    channel_t channel;              // channel type
    char pathname[PIRATE_LEN_NAME]; // optional device path
} pirate_channel_t;

static pirate_channel_t readers[PIRATE_NUM_CHANNELS] = {{0, PIPE, ""}};
static pirate_channel_t writers[PIRATE_NUM_CHANNELS] = {{0, PIPE, ""}};

// gaps descriptors must be opened from smallest to largest
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
    if (fd > 0) {
        return gd;
    }

    switch (channels[gd].channel) {
        case PIPE:
            /* Create a named pipe, if one does not exist */
            snprintf(pathname, sizeof(pathname) - 1, PIRATE_FILENAME, gd);
            rv = mkfifo(pathname, 0660);
            if ((rv == -1) && (errno != EEXIST)) {
                return -1;
            }
            break;
        case DEVICE:
            if (strnlen(channels[gd].pathname, PIRATE_LEN_NAME) == 0) {
                errno = EINVAL;
                return -1;
            }
            strncpy(pathname, channels[gd].pathname, PIRATE_LEN_NAME);
            break;
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
    pirate_channel_t* channels;
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

    channels[gd].fd = 0;
    return close(fd);
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    int fd;

    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = EBADF;
        return -1;
    }

    fd = readers[gd].fd;
    if (fd <= 0) {
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
    if (fd <= 0) {
        errno = EBADF;
        return -1;
    }

    return write(fd, buf, count);
}

int pirate_fcntl0(int gd, int flags, int cmd) {
    pirate_channel_t* channels;
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

int pirate_fcntl1(int gd, int flags, int cmd, int arg) {
    pirate_channel_t* channels;
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

int pirate_set_pathname(int gd, char *pathname) {
    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = EBADF;
        return -1;
    }
    strncpy(readers[gd].pathname, pathname, PIRATE_LEN_NAME);
    strncpy(writers[gd].pathname, pathname, PIRATE_LEN_NAME);
    return 0;
}

int pirate_get_pathname(int gd, char *pathname) {
    if (gd < 0 || gd >= PIRATE_NUM_CHANNELS) {
        errno = EBADF;
        return -1;
    }
    strncpy(pathname, readers[gd].pathname, PIRATE_LEN_NAME);
    return 0;
}