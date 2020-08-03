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

#include "tweetnacl.h"
#include "base64.h"

#define KEY_BYTES crypto_secretbox_KEYBYTES
#define NONCE_BYTES crypto_secretbox_NONCEBYTES
#define ZERO_BYTES crypto_secretbox_ZEROBYTES
#define BOX_ZERO_BYTES crypto_secretbox_BOXZEROBYTES
#define DELTA_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)
#define ZERO_AND_DELTA_BYTES (ZERO_BYTES + DELTA_BYTES)

#if DELTA_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = {0};

static const unsigned char key2[KEY_BYTES] = "secret key 2";
static unsigned char nonce2[NONCE_BYTES] = {0};

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

int decrypt1(char *input, size_t len, char *output) {
    int rv = crypto_secretbox_open((unsigned char*) output, (unsigned char*) input, len, nonce1, key1);
    increment_nonce(nonce1, NONCE_BYTES);
    return rv;
}

int decrypt2(char *input, size_t len, char *output) {
    int rv = crypto_secretbox_open((unsigned char*) output, (unsigned char*) input, len, nonce2, key2);
    increment_nonce(nonce2, NONCE_BYTES);
    return rv;
}

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int main_decryption(char *buffer1, char *buffer2, char *encoded) {
    size_t mlen;
    char *newline;
    const size_t read_length = base64_size;

    char *success = fgets(encoded, read_length, stdin);
    if (success == NULL) {
        return -1;
    }
    // strip trailing newline
    newline = strrchr(encoded, '\n');
    if (newline != NULL) {
        *newline = 0;
    }
    mlen = base64_decode(buffer1 + BOX_ZERO_BYTES, encoded) + BOX_ZERO_BYTES;

    if (decrypt2(buffer1, mlen, buffer2) < 0) {
        return -1;
    }
    mlen -= DELTA_BYTES;
    if (decrypt1(buffer2 + DELTA_BYTES, mlen, buffer1) < 0) {
        return -1;
    }
    buffer1[mlen] = 0;

    printf("%s\n", buffer1 + ZERO_BYTES);
    return 0;
}

int main() {
    char *buffer1 = calloc(double_encryption_size, 1);
    char *buffer2 = calloc(double_encryption_size, 1);
    char *encoded = calloc(base64_size, 1);
    int rv = main_decryption(buffer1, buffer2, encoded);
    free(buffer1);
    free(buffer2);
    free(encoded);
    return rv;
}
