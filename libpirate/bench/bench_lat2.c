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

#define _GNU_SOURCE

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libpirate.h"

int test_gd1 = -1, test_gd2 = -1, sync_gd = -1;
uint64_t nbytes;
size_t message_len, signal_len = 64;
char message[80];
unsigned char *read_buffer, *write_buffer;

int bench_lat_setup(char *argv[], int test_flag1, int test_flag2, int sync_flags);
void bench_lat_close(char *argv[]);

int run(int argc, char *argv[]) {
    ssize_t rv;
    uint64_t readcount = 0, writecount = 0, iter, delta;
    struct timespec start, stop;

    if (argc != 6) {
        printf("./bench_lat1 [test channel 1] [test channel 2] [sync channel] [message length] [nbytes]\n\n");
        return 1;
    }

    if (bench_lat_setup(argv, O_RDONLY, O_WRONLY, O_WRONLY)) {
        return 1;
    }

    if (signal_len > nbytes) {
        signal_len = nbytes;
    }

    rv = pirate_write(sync_gd, write_buffer, signal_len);
    if (rv < 0) {
        perror("Sync channel initial write error");
        return 1;
    }

    rv = pirate_read(test_gd1, read_buffer, signal_len);
    if (rv < 0) {
        perror("Test channel initial read error");
        return 1;
    }

    iter = nbytes / message_len;

    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      perror("clock_gettime start");
      return 1;
    }
    for (uint64_t i = 0; i < iter; i++) {
        ssize_t count;

        count = message_len;
        while (count > 0) {
            rv = pirate_write(test_gd2, write_buffer + writecount, count);
            if (rv < 0) {
                perror("Test channel 2 write error");
                return 1;
            }
            writecount += rv;
            count -= rv;
        }

        count = message_len;
        while (count > 0) {
            rv = pirate_read(test_gd1, read_buffer + readcount, count);
            if (rv < 0) {
                perror("Test channel 1 read error");
                return 1;
            }
            readcount += rv;
            count -= rv;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &stop) < 0) {
      perror("clock_gettime stop");
      return 1;
    }

    rv = pirate_write(sync_gd, write_buffer, message_len);
    if (rv < 0) {
        perror("Sync channel terminal write error");
        return 1;
    }

    for (uint64_t i = 0; i < nbytes; i++) {
        if (read_buffer[i] != (unsigned char) (i % UCHAR_MAX)) {
            fprintf(stderr, "At position %zu expected %zu and read character %d\n",
                i, (i % UCHAR_MAX), (int) read_buffer[i]);
            return 1;
        }
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000ll +
             (stop.tv_nsec - start.tv_nsec));

    printf("average latency: %li ns\n", delta / (iter * 2));

    return 0;
}

int main(int argc, char *argv[]) {
    int rv = run(argc, argv);
    bench_lat_close(argv);
    return rv;
}
