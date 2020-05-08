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

#include <argp.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "common.h"
#include "libpirate.h"

#define DEFAULT_PACKET_DELAY_US 1000000

typedef struct {
    uint32_t delay_us;
    channel_test_t common;
} writer_t;

static struct argp_option options[] = {
    { "delay",   'd', "US",   0, "Inter-packet delay",               0 },
    COMMON_OPTIONS,
    { NULL, 0, NULL, 0, NULL, 0 }
};


static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    writer_t *writer = (writer_t*) state->input;

    if (parse_common_options(key, arg, &writer->common, state) == 1) {
        return 0;
    }

    switch (key) {

    case 'd':
        writer->delay_us = strtol(arg, NULL, 10);
        break;

    default:
        break;
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
    if (test_data_init(&writer->common.data, writer->common.verbosity) != 0) {
        log_msg(ERROR, "Failed to initialize test data");
        return 0;
    }

    /* Open GAPS channel for writing */
    writer->common.gd = pirate_open_parse(writer->common.conf, O_WRONLY);
    if (writer->common.gd < 0) {
        log_msg(ERROR, "Failed to open GAPS channel for writing");
        return -1;
    }

    if(pirate_get_channel_param(writer->common.gd, &param) != 0) {
        log_msg(ERROR, "Failed to get GAPS channel parameters");
        return -1;
    }

    fd = pirate_get_fd(writer->common.gd);
    if (fd <= 0) {
        log_msg(ERROR, "Failed to get channel descriptor");
        return -1;
    }

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
    test_data_term(&writer->common.data);

    /* Close the test cahnnel */
    return pirate_close(writer->common.gd);
}

static int writer_run_perf_test(writer_t *writer) {
    const uint8_t *wr_buf = writer->common.data.buf;
    const uint32_t wr_len = writer->common.data.perf.len;
    volatile msg_index_t *idx = (volatile msg_index_t *) wr_buf;

    const struct timespec ts = {
        .tv_sec = writer->delay_us / 1000000,
        .tv_nsec = (writer->delay_us % 1000000) * 1000
    };

    if (writer->common.verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test START");
    }

    for (*idx = 0; *idx < writer->common.data.perf.count; (*idx) +=2) {
        int rv = pirate_write(writer->common.gd, wr_buf, wr_len);
        if ((rv < 0) || (((size_t) rv) != wr_len)) {
            log_msg(ERROR, "Failed to write on GAPS channel");
            return -1;
        }

        if (writer->delay_us != 0) {
            nanosleep(&ts, NULL);
        }
    }
    
    if (writer->common.verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test DONE");
    }

    return 0;
}

static int writer_run_seq_test(writer_t *writer) {
    uint32_t wr_len = 0;
    uint32_t done = 0;
    ssize_t rv;
    const uint8_t *wr_buf = writer->common.data.buf;

    const struct timespec ts = {
        .tv_sec = writer->delay_us / 1000000,
        .tv_nsec = (writer->delay_us % 1000000) * 1000
    };

    do {
        test_data_get_next_len(&writer->common.data, &wr_len, &done);
        if (writer->common.verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Writing %u bytes", wr_len);
            if (writer->common.verbosity >= VERBOSITY_MAX) {
                print_hex("Test data", wr_buf, wr_len);
            }
        }

        rv = pirate_write(writer->common.gd, wr_buf, wr_len);
        if ((rv < 0) || (((size_t) rv) != wr_len)) {
            log_msg(ERROR, "Failed to write on GAPS channel");
            return -1;
        }

        if (test_data_save(&writer->common.data, wr_len) != 0) {
            log_msg(ERROR, "Failed to save test data");
            return -1;
        }

        if ((writer->delay_us != 0) && !done) {
            nanosleep(&ts, NULL);
        }
    } while (done == 0);

    return 0;
}

static int writer_run(writer_t *writer) {
    if (writer->common.data.perf.enabled) {
        return writer_run_perf_test(writer);
    } else {
        return writer_run_seq_test(writer);
    }
}

int main(int argc, char *argv[]) {
    int rv = -1;
    writer_t writer = {
        .delay_us  = DEFAULT_PACKET_DELAY_US,
        .common      = TEST_INIT("wr")
    };

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
