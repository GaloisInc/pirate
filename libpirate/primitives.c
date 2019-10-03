#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "primitives.h"

typedef struct {
    int gd;             // gaps descriptor
    int fd;             // pipe file descriptor
    int flags;          // pipe open flags
} pirate_channel_t;

static pirate_channel_t* channels[PIRATE_MAX_CHANNEL + 1] = {0};

int pirate_open(int gd, int flags) {
    pirate_channel_t* ch;
    int rv;
    char pathname[PIRATE_LEN_NAME];

    if (gd < 0 || gd > PIRATE_MAX_CHANNEL) {
        errno = EBADF;
        return -1;
    }

    if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
        errno = EINVAL;
        return -1;
    }

    /* Prevent double-open */
    ch = channels[gd];
    if (ch != NULL) {
        errno = EBUSY;
        return -1;
    }

    /* Allocate a new hash entry */
    ch = (pirate_channel_t* )calloc(1, sizeof(pirate_channel_t));
    if (ch == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Create a named pipe, if one does not exist */
    snprintf(pathname, sizeof(pathname) - 1, PIRATE_FILENAME, gd);
    rv = mkfifo(pathname, 0660);
    if ((rv == -1) && (errno != EEXIST)) {
        return -1;
    }

    /* Open the pipe: reader will block until writer opens its side */
    ch->gd = gd;
    ch->flags = flags;
    ch->fd = open(pathname, O_RDWR);
    if (ch->fd < 0) {
        free(ch);
        return -1;
    }

    /* Success */
    channels[gd] = ch;
    return gd;
}

int pirate_close(int gd) {
    int rv = 0;
    pirate_channel_t* ch;

    if (gd < 0 || gd > PIRATE_MAX_CHANNEL) {
        errno = EBADF;
        return -1;
    }

    ch = channels[gd];
    if (ch == NULL) {
        errno = ENODEV;
        return -1;
    }

    rv = close(ch->fd);
    free(ch);
    channels[gd] = NULL;

    return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    pirate_channel_t* ch;

    if (gd < 0 || gd > PIRATE_MAX_CHANNEL) {
        errno = EBADF;
        return -1;
    }

    ch = channels[gd];
    if (ch == NULL) {
        errno = EBADF;
        return -1;
    }

    if (ch->flags != O_RDONLY) {
        errno = EBADF;
        return -1;
    }

    return read(ch->fd, buf, count);
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
    pirate_channel_t* ch;

    if (gd < 0 || gd > PIRATE_MAX_CHANNEL) {
        errno = EBADF;
        return -1;
    }

    ch = channels[gd];
    if (ch == NULL) {
        errno = EBADF;
        return -1;
    }

    if (ch->flags != O_WRONLY) {
        errno = EBADF;
        return -1;
    }

    return write(ch->fd, buf, count);
}
