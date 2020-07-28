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
#include <time.h>
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

static const int input_size = 80;
static const int encryption_size = input_size + ZERO_BYTES;
static const int double_encryption_size = encryption_size + DELTA_BYTES;
static const int base64_size = (((double_encryption_size / 3) + 1) * 4);

int encrypt1_write, encrypt1_read;
int encrypt2_write, encrypt2_read;

int main_encryption(char *buffer1, char *buffer2, char *encoded) {
    ssize_t mlen;
    uint32_t mlen_n;
    char *newline, *read_offset = buffer1 + ZERO_BYTES;
    const size_t read_length = input_size;

    encrypt1_write = writer_open(8080);
    encrypt1_read = reader_open(8081);
    encrypt2_write = writer_open(8082);
    encrypt2_read = reader_open(8083);

    if (encrypt1_write < 0) {
        perror("writer_open(8080)");
        return -1;
    }

    if (encrypt1_read < 0) {
        perror("reader_open(8081)");
        return -1;
    }

    if (encrypt2_write < 0) {
        perror("writer_open(8082)");
        return -1;
    }

    if (encrypt2_read < 0) {
        perror("reader_open(8083)");
        return -1;
    }

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        return -1;
    }
    // strip trailing newline
    newline = strrchr(read_offset, '\n');
    if (newline != NULL) {
        *newline = 0;
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;
    mlen_n = htonl(mlen);

    ssize_t rv = send(encrypt1_write, &mlen_n, sizeof(mlen_n), 0);
    if (rv != sizeof(mlen_n)) {
        perror("send(encrypt1_write...)");
        return -1;
    }
    rv = send(encrypt1_write, buffer1, mlen, 0);
    if (rv != mlen) {
        perror("send(encrypt1_write...)");
        return -1;
    }
    rv = recv(encrypt1_read, buffer2 + DELTA_BYTES, mlen, 0);
    if (rv != mlen) {
        perror("recv(encrypt1_read...)");
        return -1;
    }
    mlen += DELTA_BYTES;
    mlen_n = htonl(mlen);
    rv = send(encrypt2_write, &mlen_n, sizeof(mlen_n), 0);
    if (rv != sizeof(mlen_n)) {
        perror("send(encrypt2_write...)");
        return -1;
    }
    rv = send(encrypt2_write, buffer2, mlen, 0);
    if (rv != mlen) {
        perror("send(encrypt2_write...)");
        return -1;
    }
    rv = recv(encrypt2_read, buffer1, mlen, 0);
    if (rv != mlen) {
        perror("recv(encrypt2_read...)");
        return -1;
    }

    base64_encode(encoded, buffer1 + BOX_ZERO_BYTES, mlen - BOX_ZERO_BYTES);
    printf("%s\n", encoded);

    return 0;
}

int main() {
    char *buffer1 = calloc(double_encryption_size, 1);
    char *buffer2 = calloc(double_encryption_size, 1);
    char *encoded = calloc(base64_size, 1);
    int rv = main_encryption(buffer1, buffer2, encoded);
    free(buffer1);
    free(buffer2);
    free(encoded);
    close(encrypt1_write);
    close(encrypt1_read);
    close(encrypt2_write);
    close(encrypt2_read);
    return rv;
}
