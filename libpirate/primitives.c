#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "uthash.h"
#include "primitives.h"

#define PIRATE_INOTIFY_SIZE ( sizeof (struct inotify_event) + PIRATE_LEN_NAME )

struct gd_to_fd {
    int gd; // key: gaps descriptor
    int fd;
    int flags;
    UT_hash_handle hh; // makes this structure hashable
};

static struct gd_to_fd *gd_hash = NULL;

static int pirate_open_write(char *pathname);
static int pirate_open_read(char *pathname);

int pirate_open(int gd, int flags) {
    struct gd_to_fd *gd_entry;
    int rv, fd = -1;
    char pathname[PIRATE_LEN_NAME];

    if (gd < 0) {
        errno = EBADF;
        return -1;
    }
    if ((flags != O_RDONLY) && (flags != O_WRONLY)) {
        errno = EINVAL;
        return -1;
    }

    HASH_FIND_INT(gd_hash, &gd, gd_entry);
    if (gd_entry != NULL) {
        fd = gd_entry->fd;
        switch (gd_entry->flags) {
            case O_RDWR:
                return gd;
            case O_WRONLY:
                if (flags == O_WRONLY) {
                    return gd;
                }
                gd_entry->flags = O_RDWR;
                break;
            case O_RDONLY:
                if (flags == O_RDONLY) {
                    return gd;
                }
                gd_entry->flags = O_RDWR;
                break;
            default:
                // this should never happen
                errno = EIO;
                return -1;
        }
    }

    sprintf(pathname, PIRATE_FILENAME, gd);
    if (flags == O_WRONLY) {
        rv = pirate_open_write(pathname);
    } else { // flags == O_RDONLY
        rv = pirate_open_read(pathname);
    }
    if (rv < 0) {
        return rv;
    }
    if (fd == -1) {
        // Opening a FIFO for reading normally blocks
        // until some other process opens the same FIFO
        // for writing, and vice versa.
        //
        // Under Linux, opening a FIFO for read
        // and write will succeed without blocking.
        // POSIX leaves this behavior undefined.
        fd = open(pathname, O_RDWR);
        if (fd < 0) {
            return fd;
        }
        gd_entry = malloc(sizeof(struct gd_to_fd));
        if (gd_entry == NULL)
        {
            errno = ENOMEM;
            return -1;
        }
        gd_entry->fd = fd;
        gd_entry->gd = gd;
        gd_entry->flags = flags;
        // not thread-safe
        HASH_ADD_INT(gd_hash, gd, gd_entry);
    }
    return gd;
}

// O_WRONLY unique operations for pirate_open()
static int pirate_open_write(char *pathname) {
    return mkfifo(pathname, 0660);
}

// O_RDONLY unique operations for pirate_open()
static int pirate_open_read(char *pathname) {
    int notify, watch, rv = 0;
    char buffer[PIRATE_INOTIFY_SIZE];

    if (access(pathname, F_OK) == 0) {
        return 0;
    }
    notify = inotify_init();
    if (notify < 0) {
        return notify;
    }
    watch = inotify_add_watch(notify, "/tmp", IN_CREATE);
    if (watch < 0) {
        close(notify);
        return watch;
    }
    for (;;) {
        int nread;
        if (access(pathname, F_OK) == 0) {
            break;
        }
        nread = read(watch, buffer, PIRATE_INOTIFY_SIZE);
        if (nread < 0) {
            rv = nread;
            break;
        }
    }
    close(watch);
    close(notify);
    return rv;
}

ssize_t pirate_read(int gd, void *buf, size_t count) {
    struct gd_to_fd *gd_entry;

    HASH_FIND_INT(gd_hash, &gd, gd_entry);
    if (gd_entry == NULL) {
        errno = EBADF;
        return -1;
    }
    if ((gd_entry->flags != O_RDONLY) && (gd_entry->flags != O_RDWR)) {
        errno = EBADF;
        return -1;
    }
    return read(gd_entry->fd, buf, count);
}

ssize_t pirate_write(int gd, const void *buf, size_t count) {
    struct gd_to_fd *gd_entry;

    HASH_FIND_INT(gd_hash, &gd, gd_entry);
    if (gd_entry == NULL) {
        errno = EBADF;
        return -1;
    }
    if ((gd_entry->flags != O_WRONLY) && (gd_entry->flags != O_RDWR)) {
        errno = EBADF;
        return -1;
    }
    return write(gd_entry->fd, buf, count);
}

int pirate_close(int gd) {
    int rv = 0;
    struct gd_to_fd *gd_entry;
    char pathname[PIRATE_LEN_NAME];

    HASH_FIND_INT(gd_hash, &gd, gd_entry);
    if (gd_entry == NULL) {
        errno = EBADF;
        return -1;
    }
    switch (gd_entry->flags) {
        case O_RDWR:
            gd_entry->flags = O_RDONLY;
            break;
        case O_WRONLY:
            rv = close(gd_entry->fd);
            // not thread-safe
            HASH_DEL(gd_hash, gd_entry);
            free(gd_hash);
            break;
        case O_RDONLY:
            rv = close(gd_entry->fd);
            sprintf(pathname, PIRATE_FILENAME, gd_entry->gd);
            remove(pathname);
            // not thread-safe
            HASH_DEL(gd_hash, gd_entry);
            free(gd_hash);
            break;
    }
    return rv;
}
