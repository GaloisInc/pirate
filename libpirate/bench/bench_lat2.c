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
#include "bench_lat.h"

int run(bench_lat_t *bench) {
    ssize_t rv;
    const uint32_t iter = bench->nbytes / bench->message_len;
    uint64_t delta;
    struct timespec start, stop;
    uint8_t signal = 1;
    const struct timespec ts = {
        .tv_sec = bench->tx_delay_ns / 1000000000,
        .tv_nsec = (bench->tx_delay_ns % 1000000000)
    };

    /* Open and configure synchronization and test channels */
    if (bench_lat_setup(bench, O_RDONLY, O_WRONLY, O_RDONLY, O_WRONLY)) {
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

    for (uint64_t i = 0; i < iter; i++) {
        size_t count;

        count = bench->message_len;
        while (count > 0) {
            // Sleep must happen before the write
            // to slow down the write.
            // If the sleep happens before the read
            // then we are just replacing some blocking
            // read time with sleep time.
            if (bench->tx_delay_ns >= 1000000000) {
                nanosleep(&ts, NULL);
            } else if (bench->tx_delay_ns > 0) {
                bench_lat_busysleep(bench->tx_delay_ns);
            }
            rv = pirate_write(bench->test_ch2.gd, bench->write_buffer, count);
            if (rv < 0) {
                perror("Test channel 2 write error");
                return -1;
            }
            count -= rv;
        }

        count = bench->message_len;
        while (count > 0) {
            rv = pirate_read(bench->test_ch1.gd, bench->read_buffer, count);
            if (rv < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    perror("Test channel 1 read timeout");
                } else {
                    perror("Test channel 1 read error");
                }
                return -1;
            }
            count -= rv;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &stop) < 0) {
        perror("clock_gettime stop");
        return -1;
    }

    rv = pirate_write(bench->sync_ch2.gd, &signal, sizeof(signal));
    if (rv < 0) {
        perror("Sync channel 2 terminating write error");
        return -1;
    }

    delta = ((stop.tv_sec - start.tv_sec) * 1000000000ll +
             (stop.tv_nsec - start.tv_nsec));

    printf("average latency: %li ns\n", delta / (iter * 2));

    return 0;
}

int main(int argc, char *argv[]) {
    bench_lat_t bench;
    memset(&bench, 0, sizeof(bench));
    parse_args(argc, argv, &bench);
    int rv = run(&bench);
    bench_lat_close(&bench);
    return rv;
}
