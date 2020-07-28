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
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_BYTES < 0
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
 * 
 * TODO: prepend the nonces to the output string
 **/

void encrypt1(char *input, size_t len, char *output) {
    crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
    increment_nonce(nonce1, NONCE_BYTES);
}

int encrypt1_read, encrypt1_write;

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;

int main_encryption(char *buffer1, char *buffer2) {
    ssize_t mlen;
    uint32_t mlen_n;

    encrypt1_read = reader_open(8080);
    encrypt1_write = writer_open(8081);

    if (encrypt1_read < 0) {
        perror("reader_open(8080)");
        return -1;
    }

    if (encrypt1_write < 0) {
        perror("writer_open(8081)");
        return -1;
    }

    ssize_t rv = recv(encrypt1_read, &mlen_n, sizeof(mlen_n), 0);
    test_rv(rv, sizeof(mlen_n), "recv(encrypt1_read, &mlen_n...)");
    mlen = ntohl(mlen_n);
    rv = recv(encrypt1_read, buffer1, mlen, 0);
    test_rv(rv, mlen, "recv(encrypt1_read, input...)");
    encrypt1(buffer1, mlen, buffer2);
    rv = send(encrypt1_write, buffer2, mlen, 0);
    test_rv(rv, mlen, "send(encrypt1_write, output...)");

    return 0;
}

int main() {
    char *buffer1 = calloc(double_encryption_size, 1);
    char *buffer2 = calloc(double_encryption_size, 1);
    int rv = main_encryption(buffer1, buffer2);
    free(buffer1);
    free(buffer2);
    close(encrypt1_write);
    close(encrypt1_read);
    return rv;
}
