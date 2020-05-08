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

#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bench_thr.h"

#define tscmp(a, b, CMP)                             \
  (((a)->tv_sec == (b)->tv_sec) ?                    \
   ((a)->tv_nsec CMP (b)->tv_nsec) :                 \
   ((a)->tv_sec CMP (b)->tv_sec))

#define tsadd(a, b, result)                          \
  do {                                               \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;    \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec; \
    if ((result)->tv_nsec >= 1000000000) {           \
      ++(result)->tv_sec;                            \
      (result)->tv_nsec -= 1000000000;               \
    }                                                \
  } while (0)

int busysleep(uint32_t nanoseconds)
{
    struct timespec now;
    struct timespec then;
    struct timespec start;
    struct timespec sleep;

    if (nanoseconds >= 1000000000) {
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    now = start;
    sleep.tv_sec = 0;
    sleep.tv_nsec = nanoseconds;
    tsadd(&start, &sleep, &then);
    while (tscmp(&now, &then, <)) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    }
    return 0;
}

int run(bench_thr_t *bench) {
    ssize_t rv;
    const uint32_t iter = bench->nbytes / bench->message_len;
    uint64_t write_off = 0;
    uint8_t signal = 1;

    const struct timespec ts = {
        .tv_sec = bench->tx_delay_ns / 1000000000,
        .tv_nsec = (bench->tx_delay_ns % 1000000000)
    };

    /* Open and configure synchronization and test channels */
    if (bench_thr_setup(bench, O_WRONLY, O_WRONLY, O_RDONLY)) {
        return -1;
    }

    /* Initialize the write buffer */
    for (uint32_t i = 0; i < iter; i++) {
        uint64_t offset = i * bench->message_len;
        if (bench->message_len < 4) {
            for (uint64_t j = 0; j < bench->message_len; j++) {
                bench->buffer[offset + j] = (unsigned char)(j & 0xFF);
            }
        } else {
            uint32_t sequence = i + 1;
            bench->buffer[offset] = sequence & 0xFF;
            bench->buffer[offset + 1] = (sequence >> 8) & 0xFF;
            bench->buffer[offset + 2] = (sequence >> 16) & 0xFF;
            bench->buffer[offset + 3] = (sequence >> 24) & 0xFF;
            for (uint64_t j = 4; j < bench->message_len; j++) {
                bench->buffer[offset + j] = (unsigned char)(j & 0xFF);
            }
        }
    }

    rv = pirate_read(bench->sync_ch2.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 initial read error");
        return -1;
    }

    rv = pirate_write(bench->sync_ch1.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 1 initial write error");
        return -1;
    }

    for (uint32_t i = 0; i < iter; i++) {
        size_t count = bench->message_len;
        while (count > 0) {
            rv = pirate_write(bench->test_ch.gd, bench->buffer + write_off, count);
            if (rv < 0) {
                perror("Test channel write error");
                return -1;
            }
            write_off += rv;
            count -= rv;
            if (bench->tx_delay_ns >= 1000000000) {
                nanosleep(&ts, NULL);
            } else if (bench->tx_delay_ns > 0) {
                busysleep(bench->tx_delay_ns);
            }
        }
    }

    /* Read sync from the reader */
    rv = pirate_read(bench->sync_ch2.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 terminating read error");
        return -1;
    }

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



