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
#define DELTA_ZERO_BYTES (ZERO_BYTES - BOX_ZERO_BYTES)

#if DELTA_ZERO_BYTES < 0
#error "crypto_secretbox_ZEROBYTES is assumed to be >= crypto_secretbox_BOXZEROBYTES"
#endif

int main() {
    char input[80 + DELTA_ZERO_BYTES] = {0};
    char output[80 + DELTA_ZERO_BYTES] = {0};
    char encoded[120] = {0};

    ssize_t mlen;
    uint32_t mlen_n;
    char* read_offset = input + ZERO_BYTES;
    const size_t read_length = 80 - ZERO_BYTES;

    int encrypt1_write = writer_open(8080);
    int encrypt1_read = reader_open(8081);
    int encrypt2_write = writer_open(8082);
    int encrypt2_read = reader_open(8083);

    if (encrypt1_write < 0) {
        perror("writer_open(8080)");
        exit(1);
    }

    if (encrypt1_read < 0) {
        perror("reader_open(8081)");
        exit(1);
    }

    if (encrypt2_write < 0) {
        perror("writer_open(8082)");
        exit(1);
    }

    if (encrypt2_read < 0) {
        perror("reader_open(8083)");
        exit(1);
    }

    char *success = fgets(read_offset, read_length, stdin);
    if (success == NULL) {
        exit(1);
    }
    mlen = strnlen(read_offset, read_length) + ZERO_BYTES;
    mlen_n = htonl(mlen);

    // Execute 100_000 iterations to identify any race conditions
    // in concurrent implementations of the challenge problem.
    for (int i = 0; i < 100000; i++) {
        ssize_t rv = send(encrypt1_write, &mlen_n, sizeof(mlen_n), 0);
        if (rv != sizeof(mlen_n)) {
            perror("send(encrypt1_write...)");
            exit(1);
        }
        rv = send(encrypt1_write, input, mlen, 0);
        if (rv != mlen) {
            perror("send(encrypt1_write...)");
            exit(1);
        }
        rv = recv(encrypt1_read, output + DELTA_ZERO_BYTES, mlen, 0);
        if (rv != mlen) {
            perror("recv(encrypt1_read...)");
            exit(1);
        }
        rv = send(encrypt2_write, &mlen_n, sizeof(mlen_n), 0);
        if (rv != sizeof(mlen_n)) {
            perror("send(encrypt2_write...)");
            exit(1);
        }
        rv = send(encrypt2_write, output, mlen, 0);
        if (rv != mlen) {
            perror("send(encrypt2_write...)");
            exit(1);
        }
        rv = recv(encrypt2_read, input + DELTA_ZERO_BYTES, mlen, 0);
        if (rv != mlen) {
            perror("recv(encrypt2_read...)");
            exit(1);
        }
    }

    base64_encode(encoded, input + ZERO_BYTES, mlen - ZERO_BYTES);
    printf("%s\n", encoded);

    close(encrypt1_write);
    close(encrypt1_read);
    close(encrypt2_write);
    close(encrypt2_read);

    return 0;
}
