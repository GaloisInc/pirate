#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tweetnacl.h"
#include "base64.h"
#include "challenge_socket_common.h"

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
