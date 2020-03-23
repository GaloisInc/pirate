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
#include "common.h"
#include "libpirate.h"

typedef channel_test_t reader_t;

extern int gaps_channel;

static struct argp_option options[] = {
    COMMON_OPTIONS,
    { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    reader_t *reader = (reader_t*) state->input;

    if (parse_common_options(key, arg, reader, state, O_RDONLY) == 1) {
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
    if (test_data_init(&reader->data, reader->verbosity) != 0) {
        log_msg(ERROR, "Failed to initialize test data");
        return 0;
    }

    return 0;
}

static int reader_term(reader_t *reader) {
    test_data_term(&reader->data);
    return pirate_close(gaps_channel);
}

static int reader_run(reader_t *reader) {
    int rv = 0;
    uint32_t exp_len = 0;
    uint32_t done = 0;
    uint32_t buf_len = reader->data.len.stop - 1;
    const char *out_dir = reader->data.out_dir;
    const uint8_t *exp_buf = reader->data.buf;

    uint8_t *rd_buf = (uint8_t *)malloc(buf_len);
    if (rd_buf == NULL) {
        log_msg(INFO, "Failed to allocate read buffer");
        return -1;
    }

    do {
        test_data_get_next_len(&reader->data, &exp_len, &done);
        if (reader->verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Waiting for %u bytes", exp_len);
            if (reader->verbosity >= VERBOSITY_MAX) {
                print_hex("Expecting data", reader->data.buf, exp_len);
            }
        }

        ssize_t rd_len = pirate_read(gaps_channel, rd_buf, buf_len);
        if (rd_len < 0) {
            log_msg(ERROR, "Failed to read GAPS data");
            rv = -1;
            break;
        }

        if (reader->verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Message received: %u bytes", rd_len);
            if (reader->verbosity >= VERBOSITY_MAX) {
                print_hex("Received data", rd_buf, exp_len);
            }
        }

        if (rd_len != exp_len) {
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

        if (reader->verbosity >= VERBOSITY_MIN) {
            log_msg(INFO, "Data content verified");
        }

        if (test_data_save(&reader->data, rd_len) != 0) {
            log_msg(ERROR, "Failed to save test data");
            rv = -1;
            break;
        }
    } while (done == 0);

    free(rd_buf);
    return rv;
}

int main(int argc, char *argv[]) {
    int rv = -1;
    reader_t reader = TEST_INIT("rd");

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
