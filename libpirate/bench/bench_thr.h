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

#ifndef _PIRATE_BENCH_THR_H
#define _PIRATE_BENCH_THR_H

#include <argp.h>
#include <stdint.h>
#include "libpirate.h"

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

typedef struct {
    int gd;
    const char *config;
} bench_channel_t;

typedef struct {
    bench_channel_t test_ch;
    bench_channel_t sync_ch1;
    bench_channel_t sync_ch2;
    uint64_t nbytes;
    size_t message_len;
    int validate;
    uint64_t tx_delay_ns;
    uint32_t rx_timeout_s;
    uint8_t *buffer;
    uint8_t *bitvector;
    char err_msg[256];
} bench_thr_t;

void parse_args(int argc, char *argv[], bench_thr_t *bench);
int bench_sync_write(bench_thr_t *bench, int gd);
int bench_sync_read(bench_thr_t *bench, int gd);
int bench_thr_setup(bench_thr_t *bench, int test_flags, int sync_flags1, int sync_flags2);
void bench_thr_close(bench_thr_t *bench);

#endif /* _PIRATE_BENCH_THR_H */
