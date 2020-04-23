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

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpirate.h"

int test_gd = -1, sync_gd = -1;
uint64_t nbytes;
size_t message_len, signal_len = 64;
char message[80];
unsigned char* buffer;

int bench_thr_setup(char *argv[], int test_flags, int sync_flags);
void bench_thr_close(char *argv[]);

int run(int argc, char *argv[]) {
    ssize_t rv;
    uint64_t writecount = 0, iter;

    if (argc != 5) {
        printf("./bench_thr_writer [test channel] [sync channel] [message length] [nbytes]\n\n");
        return 1;
    }

    if (bench_thr_setup(argv, O_WRONLY, O_RDONLY)) {
        return 1;
    }

    if (signal_len > nbytes) {
        signal_len = nbytes;
    }

    rv = pirate_read(sync_gd, buffer, signal_len);
    if (rv < 0) {
        perror("Sync channel initial read error");
        return 1;
    }

    for (uint64_t i = 0; i < nbytes; i++) {
        buffer[i] = (unsigned char) (i % UCHAR_MAX);
    }

    rv = pirate_write(test_gd, buffer, signal_len);
    if (rv < 0) {
        perror("Test channel initial write error");
        return 1;
    }

    iter = nbytes / message_len;

    for (uint64_t i = 0; i < iter; i++) {
        size_t count;

        count = message_len;
        while (count > 0) {
            rv = pirate_write(test_gd, buffer + writecount, count);
            if (rv < 0) {
                perror("Test channel write error");
                return 1;
            }
            writecount += rv;
            count -= rv;
        }
    }

    rv = pirate_read(sync_gd, buffer, signal_len);
    if (rv < 0) {
        perror("Sync channel terminating read error");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int rv = run(argc, argv);
    bench_thr_close(argv);
    return rv;
}
