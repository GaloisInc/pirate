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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bench_thr.h"

int run(bench_thr_t *bench) {
    ssize_t rv;
    const uint32_t iter = bench->nbytes / bench->message_len;
    uint64_t read_off = 0;
    struct timespec start, stop;
    uint64_t delta;
    int timeout = 0;
    uint8_t signal = 1;

    /* Open and configure synchronization and test channels */
    if (bench_thr_setup(bench, O_RDONLY, O_RDONLY, O_WRONLY)) {
        return -1;
    }

    rv = pirate_write(bench->sync_ch2.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 initial write error");
        return -1;
    }

    rv = pirate_read(bench->sync_ch1.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 1 initial read error");
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime start");
        return -1;
    }

    for (uint32_t i = 0; i < iter && !timeout; i++) {
        size_t count = bench->message_len;
        while (count > 0) {
            rv = pirate_read(bench->test_ch.gd, bench->buffer + read_off, count);
            if (rv < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    errno = 0;
                    timeout = 1;
                    break;
                }
                perror("Test channel read error");
                return -1;
            }
            read_off += rv;
            count -= rv;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &stop) < 0) {
        perror("clock_gettime stop");
        return -1;
    }

    /* Tell the writer that we are done */
    rv = pirate_write(bench->sync_ch2.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 terminating write error");
        return -1;
    }

    /* Validate the sequence numbers */
    if (bench->validate && (bench->message_len >= 4) && (read_off > 0)) {
        for (uint32_t i = 0; i < iter; i++) {
            uint64_t offset = i * bench->message_len;
            uint32_t sequence = 0;
            sequence += bench->buffer[offset];
            sequence += bench->buffer[offset + 1] << 8;
            sequence += bench->buffer[offset + 2] << 16;
            sequence += bench->buffer[offset + 3] << 24;
            if (sequence > iter) {
                fprintf(stderr, "Invalid sequence number at packet number %d\n", i);
            } else {
                bench->bitvector[sequence / 8] |= 1 << (sequence % 8);
            }
        }
        int first = 1;
        // sequence number starts at 1
        for (uint32_t i = 1; i <= iter; i++) {
            if (!(bench->bitvector[i / 8] & (1 << (i % 8)))) {
                if (first) {
                    fprintf(stderr, "Missing sequence number(s) ");
                    first = 0;
                }
                fprintf(stderr, "%d ", i);
            }
        }
        if (!first) {
            fprintf(stderr, "\n");
        }
    }

    /* Validate the data */
    for (uint32_t i = 0; bench->validate && (i < iter); i++) {
        uint64_t offset = i * bench->message_len;
        uint64_t start = (bench->message_len < 4) ? 0 : 4;
        for (uint64_t j = start; (j < bench->message_len) && ((offset + j) < read_off); j++) {
            uint64_t pos = offset + j;
            if (bench->buffer[pos] != (unsigned char) (j & 0xFF)) {
                fprintf(stderr, "At position %zu of packet %d expected %zu and read character %d\n",
                j, i, (j & 0xFF), (int) bench->buffer[pos]);
            }
        }
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000ll +
             (stop.tv_nsec - start.tv_nsec));
    if (timeout) {
        // subtract timeout wait
        delta -= bench->rx_timeout_s * 1000000000ll;
    }
    // 1e9 nanoseconds per second
    // 1e6 bytes per megabytes
    printf("average throughput: %f MB/s\n",
           ((1e9 / 1e6) * read_off) / delta);
    printf("drop rate: %f %%\n",
        ((bench->nbytes - read_off) / ((float) (bench->nbytes))) * 100.0);

    return 0;
}

int main(int argc, char *argv[]) {
    bench_thr_t bench;
    memset(&bench, 0, sizeof(bench));
    parse_args(argc, argv, &bench);
    int rv = run(&bench);
    bench_thr_close(&bench);
    return rv;
}
