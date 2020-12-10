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

#include <argp.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "common.h"
#include "libpirate.h"

typedef channel_test_t writer_t;


static struct argp_option options[] = {
    COMMON_OPTIONS
};

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

static int busysleep(uint32_t nanoseconds) {
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

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    writer_t *writer = (writer_t*) state->input;

    if (parse_common_options(key, arg, writer, state) == 1) {
        return 0;
    }

    return 0;
}

static void parse_args(int argc, char *argv[], writer_t *writer) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = "args ...",
        .doc = "Test utility for writing deterministic GAPS packets",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, writer);
}

static int writer_init(writer_t *writer) {
    int fd;
    pirate_channel_param_t param;    

    /* Initialize test data */
    if (test_data_init(&writer->data, writer->verbosity) != 0) {
        log_msg(ERROR, "Failed to initialize test data");
        return 0;
    }

    /* Open GAPS channel for writing */
    writer->gd = pirate_open_parse(writer->conf, O_WRONLY);
    if (writer->gd < 0) {
        log_msg(ERROR, "Failed to open GAPS channel for writing");
        return -1;
    }

    if(pirate_get_channel_param(writer->gd, &param) != 0) {
        log_msg(ERROR, "Failed to get GAPS channel parameters");
        return -1;
    }

    fd = writer->gd;

    switch (param.channel_type) {

        case TCP_SOCKET: {
            struct linger sr = {
                .l_onoff = 1,
                .l_linger = 0
            };

            if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &sr, sizeof(sr)) < 0) {
                log_msg(ERROR, "Unable to set SO_LINGER option");
                return -1;
            }
            break;
        }
        default:
            break;
    }

    return 0;
}

static int writer_term(writer_t *writer) {
    /* Release test data */
    test_data_term(&writer->data);

    /* Close the test cahnnel */
    return pirate_close(writer->gd);
}

static int writer_run_perf_test(writer_t *writer) {
    const uint8_t *wr_buf = writer->data.buf;
    const uint32_t wr_len = writer->data.perf.len;
    volatile msg_index_t *idx = (volatile msg_index_t *) wr_buf;

    const struct timespec ts = {
        .tv_sec = writer->data.delay_ns / 1000000000,
        .tv_nsec = (writer->data.delay_ns % 1000000000)
    };

    if (writer->verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test START");
    }

    for (*idx = 0; *idx < writer->data.perf.count; ++(*idx)) {
        int rv = pirate_write(writer->gd, wr_buf, wr_len);
        if ((rv < 0) || (((size_t) rv) != wr_len)) {
            log_msg(ERROR, "Failed to write on GAPS channel");
            return -1;
        }

        if (writer->data.delay_ns != 0) {
            if (writer->data.delay_ns >= 1000000000) {
                nanosleep(&ts, NULL);
            } else if (writer->data.delay_ns > 0) {
                busysleep(writer->data.delay_ns);
            }
        }
    }
    
    if (writer->verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test DONE");
    }

    return 0;
}

static int writer_run_seq_test(writer_t *writer) {
    uint32_t wr_len = 0;
    uint32_t done = 0;
    ssize_t rv;
    const uint8_t *wr_buf = writer->data.buf;

    const struct timespec ts = {
        .tv_sec = writer->data.delay_ns / 1000000000,
        .tv_nsec = (writer->data.delay_ns % 1000000000)
    };

    do {
        test_data_get_next_len(&writer->data, &wr_len, &done);
        if (writer->verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Writing %u bytes", wr_len);
            if (writer->verbosity >= VERBOSITY_MAX) {
                print_hex("Test data", wr_buf, wr_len);
            }
        }

        rv = pirate_write(writer->gd, wr_buf, wr_len);
        if ((rv < 0) || (((size_t) rv) != wr_len)) {
            log_msg(ERROR, "Failed to write on GAPS channel");
            return -1;
        }

        if (test_data_save(&writer->data, wr_len) != 0) {
            log_msg(ERROR, "Failed to save test data");
            return -1;
        }


        if ((writer->data.delay_ns != 0) && !done) {
            if (writer->data.delay_ns >= 1000000000) {
                nanosleep(&ts, NULL);
            } else if (writer->data.delay_ns > 0) {
                busysleep(writer->data.delay_ns);
            }
        }
    } while (done == 0);

    return 0;
}

static int writer_run(writer_t *writer) {
    if (writer->data.perf.enabled) {
        return writer_run_perf_test(writer);
    } else {
        return writer_run_seq_test(writer);
    }
}

int main(int argc, char *argv[]) {
    int rv = -1;
    writer_t writer = TEST_INIT("wr");

    log_msg(INFO, "Starting the writer");
    parse_args(argc, argv, &writer);

    if (writer_init(&writer) != 0) {
        log_msg(ERROR, "Failed to initialize the writer");
        writer_term(&writer);
        return -1;
    }

    if ((rv = writer_run(&writer)) != 0) {
        log_msg(ERROR, "Failed to run the writer");
    }

    if ((writer_term(&writer) != 0) && (rv == 0)) {
        log_msg(ERROR, "Failed to terminate the writer");
        rv = -1;
    }

    return rv;
}
