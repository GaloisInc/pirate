#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "tweetnacl.h"
#include "base64.h"

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_ZERO_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_ZERO_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = {0};

static void increment_nonce(unsigned char *n, const size_t nlen) {
    size_t i = 0U;
    uint_fast16_t c = 1U;
    for (; i < nlen; i++) {
        c += (uint_fast16_t) n[i];
        n[i] = (unsigned char) c;
        c >>= 8;
    }
}

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

/**
 * Assume that encrypt1() and encrypt2() use different encryption
 * algorithms that are provided by different encryption libraries.
 *
 * For simplicity they both use crypto_secretbox_xsalsa20poly1305
 * primitive by the TweetNaCl library https://tweetnacl.cr.yp.to
 **/

void encrypt1(char *input, size_t len, char *output) {
    crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
    increment_nonce(nonce1, NONCE_BYTES);
}

void test_rv(ssize_t rv, ssize_t nbytes, const char *msg) {
    if (rv == 0) {
        exit(0);
    } else if (rv != nbytes) {
        perror(msg);
        exit(1);
    }
}

int main() {
    char input[80 + DELTA_ZERO_BYTES] = {0};
    char output[80 + DELTA_ZERO_BYTES] = {0};

    ssize_t mlen;
    uint32_t mlen_n;

    int encrypt1_read = reader_open(8080);
    int encrypt1_write = writer_open(8081);

    if (encrypt1_read < 0) {
        perror("reader_open(8080)");
        exit(1);
    }

    if (encrypt1_write < 0) {
        perror("writer_open(8081)");
        exit(1);
    }

    while (1) {
        ssize_t rv = recv(encrypt1_read, &mlen_n, sizeof(mlen_n), 0);
        test_rv(rv, sizeof(mlen_n), "recv(encrypt1_read, &mlen_n...)");
        mlen = ntohl(mlen_n);
        rv = recv(encrypt1_read, input, mlen, 0);
        test_rv(rv, mlen, "recv(encrypt1_read, input...)");
        encrypt1(input, mlen, output);
        rv = send(encrypt1_write, output, mlen, 0);
        test_rv(rv, mlen, "send(encrypt1_write, output...)");
    }
    return 0;
}
