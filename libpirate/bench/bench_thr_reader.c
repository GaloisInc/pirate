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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libpirate.h"

int test_gd = -1, sync_gd1 = -1, sync_gd2 = -1;
uint64_t nbytes;
size_t message_len;
char message[80];
unsigned char* buffer;

int bench_thr_setup(char *argv[], int test_flags, int sync_flag1, int sync_flag2);
void bench_thr_close(char *argv[]);

int run(int argc, char *argv[]) {
    ssize_t rv;
    uint64_t readcount = 0, iter, delta;
    struct timespec start, stop;
    int timeout = 0;
    uint8_t signal = 1;

    if (argc != 6) {
        printf("./bench_thr_reader [test channel] [sync channel 1] [sync channel 2] [message length] [nbytes]\n\n");
        return 1;
    }

    if (bench_thr_setup(argv, O_RDONLY, O_RDONLY, O_WRONLY)) {
        return 1;
    }

    memset(buffer, 0, nbytes);

    rv = pirate_write(sync_gd2, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 initial write error");
        return 1;
    }

    rv = pirate_read(sync_gd1, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 1 initial read error");
        return 1;
    }

    iter = nbytes / message_len;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      perror("clock_gettime start");
      return 1;
    }
    for (uint64_t i = 0; i < iter && !timeout; i++) {
        size_t count;

        count = message_len;
        while (count > 0) {
            rv = pirate_read(test_gd, buffer + readcount, count);
            if (rv < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    errno = 0;
                    timeout = 1;
                    break;
                }
                perror("Test channel read error");
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

    rv = pirate_write(sync_gd2, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 terminating write error");
        return 1;
    }

    for (uint64_t i = 0; i < iter; i++) {
        for (uint64_t j = 0; j < message_len && (i * message_len + j) < readcount; j++) {
            if (buffer[i * message_len + j] != (unsigned char) (j & 0xFF)) {
                fprintf(stderr, "At position %zu of packet %zu expected %zu and read character %d\n",
                j, i, (j & 0xFF), (int) buffer[i * message_len + j]);
            }
        }
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000ll +
             (stop.tv_nsec - start.tv_nsec));
    if (timeout) {
        // subtract timeout wait
        delta -= 2 * 1000000000ll;
    }
    // 1e9 nanoseconds per second
    // 1e6 bytes per megabytes
    printf("average throughput: %f MB/s\n",
           ((1e9 / 1e6) * readcount) / delta);
    printf("drop rate: %f %%\n",
        ((nbytes - readcount) / (nbytes * 1.0)) * 100.0);

    return 0;
}

int main(int argc, char *argv[]) {
    int rv = run(argc, argv);
    bench_thr_close(argv);
    return rv;
}
