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
 * Copyright 2021 Two Six Labs, LLC.  All rights reserved.
 */

#include "libpirate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define TIMESTAMP_STR_LEN 64

static inline int get_time(char ts[TIMESTAMP_STR_LEN], const char *fmt) {
    struct timeval tv;
    struct tm *now_tm;
    int off;

    if (gettimeofday(&tv, NULL) != 0) {
        fprintf(stderr, "Failed to get the current time");
        return -1;
    }

    if ((now_tm = localtime(&tv.tv_sec)) == NULL) {
        fprintf(stderr, "Failed to convert time");
        return -1;
    }

    off = strftime(ts, TIMESTAMP_STR_LEN, fmt, now_tm);
    snprintf(ts + off, TIMESTAMP_STR_LEN - off, ".%06ld", tv.tv_usec);

    return 0;
}

void pirate_print_stats(FILE *stream, int pause_sec, pirate_atomic_bool* done, size_t ngds, int *gds) {
    pirate_stats_t *prev = (pirate_stats_t*) calloc(ngds, sizeof(pirate_stats_t));
    pirate_stats_t *next = (pirate_stats_t*) calloc(ngds, sizeof(pirate_stats_t));
    struct timespec ts;
    char timestamp[TIMESTAMP_STR_LEN];

    if (pause_sec < 1) {
        fprintf(stream, "pause_sec must be greater than or equal to 1");
        return;
    }
    if (ngds == 0) {
        fprintf(stream, "ngds must be greater than 0");
        return;
    }
    for (size_t i = 0; i < ngds; i++) {
        memcpy(&prev[i], pirate_get_stats(gds[i]), sizeof(pirate_stats_t));
    }
    ts.tv_sec = pause_sec;
    ts.tv_nsec = 0;
    while ((done == NULL) || !PIRATE_ATOMIC_LOAD(done)) {
        int rv = nanosleep(&ts, NULL);
        if (rv < 0) {
            perror("pirate_print_stats nanosleep error");
            break;
        }
        if (get_time(timestamp, "%Y-%m-%d %H:%M:%S") != 0) {
            fprintf(stream, "Failed to get the current time");
            return;
        }
        for (size_t i = 0; i < ngds; i++) {
            memcpy(&next[i], pirate_get_stats(gds[i]), sizeof(pirate_stats_t));
        }
        for (size_t i = 0; i < ngds; i++) {
            pirate_stats_t *p = &prev[i];
            pirate_stats_t *n = &next[i];  
            p->requests = n->requests - p->requests;
            p->success = n->success - p->success;
            p->errs = n->errs - p->errs;
            p->fuzzed = n->fuzzed - p->fuzzed;
            p->bytes = n->bytes - p->bytes;
            fprintf(stream, "[%s] TRACE gaps descriptor %zu\trequests\t%f\n", timestamp, i, ((float) p->requests) / ((float) pause_sec));
            fprintf(stream, "[%s] TRACE gaps descriptor %zu\tsuccess\t\t%f\n", timestamp, i, ((float) p->success) / ((float) pause_sec));
            fprintf(stream, "[%s] TRACE gaps descriptor %zu\terrs\t\t%f\n", timestamp, i, ((float) p->errs) / ((float) pause_sec));
            fprintf(stream, "[%s] TRACE gaps descriptor %zu\tfuzzed\t\t%f\n", timestamp, i, ((float) p->fuzzed) / ((float) pause_sec));
            fprintf(stream, "[%s] TRACE gaps descriptor %zu\tbytes\t\t%f\n", timestamp, i, ((float) p->bytes) / ((float) pause_sec));
        }
        memcpy(prev, next, sizeof(pirate_stats_t) * ngds);
    }
    free(prev);
    free(next);
}
