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
#define DELTA_ZERO_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_ZERO_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

static const unsigned char key1[KEY_BYTES] = "secret key 1";
static unsigned char nonce1[NONCE_BYTES] = "";

static const unsigned char key2[KEY_BYTES] = "secret key 2";
static unsigned char nonce2[NONCE_BYTES] = "";

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

void encrypt2(char *input, size_t len, char *output) {
    crypto_secretbox((unsigned char*) output, (unsigned char*) input, len, nonce2, key2);
    increment_nonce(nonce2, NONCE_BYTES);
}


int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    char input[80 + DELTA_ZERO_BYTES] = "";
    char output[80 + DELTA_ZERO_BYTES] = "";
    char encoded[120] = "";

    size_t mlen;
    char* read_offset = input + ZERO_BYTES;
    const size_t read_length = 80 - ZERO_BYTES;

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        exit(1);
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;

    // Execute 100_000 iterations to identify any race conditions
    // in concurrent implementations of the challenge problem.
    for (int i = 0; i < 100000; i++) {
        encrypt1(input, mlen, output + DELTA_ZERO_BYTES);
        encrypt2(output, mlen, input + DELTA_ZERO_BYTES);
    }

    base64_encode(encoded, input + ZERO_BYTES, mlen - ZERO_BYTES);
    printf("%s\n", encoded);

    return 0;
}
