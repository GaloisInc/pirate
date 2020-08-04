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

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int reader_open(int port) {
    int err, rv;
    int fd, server_fd;
    struct sockaddr_in addr;
    struct linger lo;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return server_fd;
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    int enable = 1;
    rv = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rv < 0) {
        err = errno;
        close(server_fd);
        errno = err;
        return rv;
    }

    rv = bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
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

    lo.l_onoff = 1;
    lo.l_linger = 0;
    rv = setsockopt(fd, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));
    if (rv < 0) {
        err = errno;
        close(fd);
        close(server_fd);
        errno = err;
        return rv;
    }

    close(server_fd);
    return fd;
}

// param port is the port of the reader
int writer_open(int port) {
    int err, rv, fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    for (;;) {
        err = errno;
        rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
        if (rv < 0) {
            if ((errno == ENOENT) || (errno == ECONNREFUSED)) {
                struct timespec req;
                errno = err;
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
    return -1;
}

void test_rv(ssize_t rv, ssize_t nbytes, const char *msg) {
    if (rv == 0) {
        exit(0);
    } else if (rv != nbytes) {
        perror(msg);
        exit(1);
    }
}
