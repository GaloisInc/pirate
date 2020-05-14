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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "common.h"
#include "libpirate.h"

typedef struct {
    uint8_t *counts;
    channel_test_t common;
} reader_t;

static struct argp_option options[] = {
    COMMON_OPTIONS
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    reader_t *reader = (reader_t*) state->input;

    if (parse_common_options(key, arg, &reader->common, state) == 1) {
        return 0;
    }

    return 0;
}

static void parse_args(int argc, char *argv[], reader_t *reader) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = "args ...",
        .doc = "Test utility for reading deterministic GAPS packets",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, reader);
}

static int reader_init(reader_t *reader) {
    int fd;
    pirate_channel_param_t param;

    /* Initialize test data */
    if (test_data_init(&reader->common.data, reader->common.verbosity) != 0) {
        log_msg(ERROR, "Failed to initialize test data");
        return -1;
    }

    if (reader->common.data.perf.enabled) {
        reader->counts = (uint8_t *)calloc(sizeof(uint8_t),
                                            reader->common.data.perf.count);
        if (reader->counts == NULL) {
            log_msg(ERROR, "Failed to allocate performance counters buffer\n");
            return -1;
        }
    }

    /* Open GAPS channel for reading */
    reader->common.gd = pirate_open_parse(reader->common.conf, O_RDONLY);
    if (reader->common.gd < 0) {
        log_msg(ERROR, "Failed to open GAPS channel for writing");
        return -1;
    }

    if(pirate_get_channel_param(reader->common.gd, &param) != 0) {
        log_msg(ERROR, "Failed to get GAPS channel parameters");
        return -1;
    }

    fd = pirate_get_fd(reader->common.gd);
    if (fd <= 0) {
        log_msg(ERROR, "Failed to get channel descriptor");
        return -1;
    }

    switch (param.channel_type) {

        case UDP_SOCKET:
        case GE_ETH: {
            struct timeval tv = {
                tv.tv_sec = 5,
                tv.tv_usec = 0
            };

            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                log_msg(ERROR, "Unable to set SO_RCVTIMEO option");
                return -1;
            }
            break;
        }

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

static int reader_term(reader_t *reader) {
    /* Release test data */
    test_data_term(&reader->common.data);

    if (reader->counts != NULL) {
        free(reader->counts);
        reader->counts = NULL;
    }

    /* Close the test cahnnel */
    return pirate_close(reader->common.gd);
}

static int reader_run_perf_test(reader_t *reader) {
    uint8_t *rd_buf = reader->common.data.buf;
    uint32_t buf_len = reader->common.data.perf.len;
    volatile msg_index_t *msg_idx = (volatile msg_index_t *) rd_buf;
    char prefix[128];

    if (reader->common.verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test START");
    }

    for (uint32_t i = 0; i < reader->common.data.perf.count; ++i) {
        ssize_t rd_len = pirate_read(reader->common.gd, rd_buf, buf_len);
        if (rd_len < 0) {
            if (errno == EAGAIN) {
                break;
            }

            log_msg(ERROR, "Failed to read GAPS data");
            return -1;
        }

        if (*msg_idx >= reader->common.data.perf.count) {
            log_msg(ERROR, "Message index is out-of-bound");
            return -1;
        }

        ++reader->counts[*msg_idx];
    }

    snprintf(prefix, sizeof(prefix) - 1, "perf_%u_%u_%zu", 
        reader->common.data.perf.len,
        reader->common.data.perf.count,
        reader->common.data.delay_ns);

    if (bin_save(prefix, reader->common.data.out_dir, reader->counts, 
                    reader->common.data.perf.count) != 0) {
            log_msg(ERROR, "Failed to safe performance counter outputs");
            return -1;
    }

    if (reader->common.verbosity >= VERBOSITY_MIN) {
        log_msg(INFO, "Performance test DONE");
    }

    return 0;
}

static int reader_run_seq_test(reader_t *reader) {
    int rv = 0;
    uint32_t exp_len = 0;
    uint32_t done = 0;
    uint32_t buf_len = reader->common.data.len.stop - 1;
    const char *out_dir = reader->common.data.out_dir;
    const uint8_t *exp_buf = reader->common.data.buf;

    uint8_t *rd_buf = (uint8_t *)malloc(buf_len);
    if (rd_buf == NULL) {
        log_msg(INFO, "Failed to allocate read buffer");
        return -1;
    }

    do {
        test_data_get_next_len(&reader->common.data, &exp_len, &done);
        if (reader->common.verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Waiting for %u bytes", exp_len);
            if (reader->common.verbosity >= VERBOSITY_MAX) {
                print_hex("Expecting data", reader->common.data.buf, exp_len);
            }
        }

        ssize_t rd_len = pirate_read(reader->common.gd, rd_buf, buf_len);
        if (rd_len < 0) {
            log_msg(ERROR, "Failed to read GAPS data");
            rv = -1;
            break;
        }

        if (reader->common.verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Message received: %u bytes", rd_len);
            if (reader->common.verbosity >= VERBOSITY_MAX) {
                print_hex("Received data", rd_buf, exp_len);
            }
        }

        if (((size_t) rd_len) != exp_len) {
            bin_save("exp", out_dir, exp_buf, exp_len);
            bin_save("got", out_dir, rd_buf, rd_len);
            log_msg(ERROR, "Expecting %u bytes, got %u. Binary files saved",
                            exp_len, rd_len);
            rv = -1;
            break;
        }

        if (memcmp(exp_buf, rd_buf, exp_len)) {
            bin_save("exp", out_dir, exp_buf, exp_len);
            bin_save("got", out_dir, rd_buf, rd_len);
            log_msg(ERROR, "Data mismatch. Binary files saved");
            rv = -1;
            break;
        }

        if (reader->common.verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Data content verified");
        }

        if (test_data_save(&reader->common.data, rd_len) != 0) {
            log_msg(ERROR, "Failed to save test data");
            rv = -1;
            break;
        }
    } while (done == 0);

    free(rd_buf);
    return rv;
}

static int reader_run(reader_t *reader) {
    if (reader->common.data.perf.enabled) {
        return reader_run_perf_test(reader);
    } else {
        return reader_run_seq_test(reader);
    }
}

int main(int argc, char *argv[]) {
    int rv = -1;
    reader_t reader = {
        .counts = NULL,
        .common = TEST_INIT("rd")
    };

    log_msg(INFO, "Starting the reader");
    parse_args(argc, argv, &reader);

    if (reader_init(&reader) != 0) {
        log_msg(ERROR, "Failed to initialize the reader");
        reader_term(&reader);
        return -1;
    }

    if ((rv = reader_run(&reader)) != 0) {
        log_msg(ERROR, "Failed to run the reader");
    }

    if ((reader_term(&reader) != 0) && (rv == 0)) {
        log_msg(ERROR, "Failed to terminate the reader");
        rv = -1;
    }

    return 0;
}
