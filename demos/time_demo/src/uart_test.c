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
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#include <argp.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "gaps_packet.h"
#include "common.h"

typedef struct {
    verbosity_t verbosity;
    uint32_t max_test_size;
    gaps_app_t app;
} uart_test_t;

typedef enum {
    COUNT = 0,
    ZEROS,
    ONES,
    ALT,
    PATTERN_COUNT
} pattern_type_t;

#define ALT_PATTERN             0xAA

typedef enum {
    REQ_TO_ECHO_WR = 0,
    REQ_TO_ECHO_RD = 1,
    ECHO_TO_REQ_WR = 2,
    ECHO_TO_REQ_RD = 3,
} test_channels_t;

#define REQ_TO_ECHO_WR_CFG     "serial,/dev/ttyUSB0"
#define REQ_TO_ECHO_RD_CFG     "serial,/dev/ttyUSB1"
#define ECHO_TO_REQ_WR_CFG     "serial,/dev/ttyUSB2"
#define ECHO_TO_REQ_RD_CFG     "serial,/dev/ttyUSB3"

#define DEFAULT_MAX_TEST_SIZE   (1 << 12)

/* Command-line options */
extern const char *argp_program_version;
static struct argp_option options[] = {
    { "test_size", 't', "BITS", 0, "Max test size 1 << BITS",  0 },
    { "verbose",   'v', NULL,   0, "Increase verbosity level", 0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    uart_test_t *uart_test = (uart_test_t *) state->input;

    switch (key) {

    case 't': {
        uint32_t bits = strtol(arg, NULL, 10);
        if (bits > 31) {
            argp_failure(state, 1, 0, "Invalid max size %s, max 31", arg);
        }
        uart_test->max_test_size = 1 << bits;
        break;
    }

    case 'v':
        if (uart_test->verbosity < VERBOSITY_MAX) {
            uart_test->verbosity++;
        }
        break;

    default:
        break;

    }

    return 0;
}

static void parse_args(int argc, char *argv[], uart_test_t *uart_test) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = NULL,
        .doc = "Utility to stress-testing UART GAPS channels",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, uart_test);
}

static void fill(uint8_t *buf, uint32_t len, pattern_type_t pattern) {
    switch(pattern) {

    case COUNT:
        for (uint32_t i = 0; i < len; ++i) {
            buf[i] = i & 0xFF;
        }
        break;

    case ZEROS:
        memset(buf, 0x00, len);
        break;

    case ONES:
        memset(buf, 0xFF, len);
        break;

    case ALT:
        memset(buf, ALT_PATTERN, len);
        break;

    case PATTERN_COUNT:
    default:
        break;
    }
}

static int validate(const uint8_t *buf, uint32_t len, pattern_type_t pattern) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t val = buf[i];
        uint8_t exp = 0;
        switch (pattern) {
        case COUNT: 
            exp = i & 0xFF;
            break;
        case ZEROS:
            exp = 0;
            break;
        case ONES:
            exp = 0xFF;
            break;
        case ALT:
            exp = ALT_PATTERN;
            break;
        case PATTERN_COUNT:
        default:
            return -1;
        }

        if (val != exp) {
            printf("Mismatch at %u: exp 0x%02X rx 0x%02X", i, exp, val);
            return -1;
        }
    }

    return 0;
}

static void *req_thread(void *arg) {
    uart_test_t *uart_test = (void *)arg;

    uint8_t *buf = (uint8_t *)malloc(uart_test->max_test_size);
    if (buf == NULL) {
        ts_log(ERROR, "Failed to allocate test buffer");
        gaps_terminate();
        return NULL;
    }

    uint32_t test_len = 1;
    pattern_type_t pattern = COUNT;

    while (gaps_running() && (test_len <= uart_test->max_test_size)) {
        fill(buf, test_len, pattern);

        ssize_t len = pirate_write(REQ_TO_ECHO_WR, buf, test_len);
        if (len != test_len) {
            ts_log(ERROR, "Failed to write echo request len = %d", len);
            gaps_terminate();
            continue;
        }

        uint8_t *rd_buf = buf;
        do {
            ssize_t rd_len = pirate_read(ECHO_TO_REQ_RD, rd_buf, len);
            if (rd_len < 0) {
                ts_log(ERROR, "Failed to read echo reply data");
                gaps_terminate();
                return NULL;
            }
            rd_buf = rd_buf + rd_len;
            len -= rd_len;
        } while (len > 0);

        if (validate(buf, test_len, pattern)) {
            ts_log(ERROR, "Failed to validate test data");
            gaps_terminate();
            continue;
        }

        if (++pattern == PATTERN_COUNT) {
            ts_log(INFO, "TEST WITH SIZE %6u - COMPLETE", test_len);
            pattern = COUNT;
            test_len <<= 1;
        }
    }

    free(buf);
    buf = NULL;
    gaps_terminate();
    return NULL;
}

static void *echo_thread(void *arg) {
    uart_test_t *uart_test = (void *)arg;
    ssize_t rd_len = 0;
    ssize_t wr_len = 0;

    uint8_t *buf = (uint8_t *)malloc(uart_test->max_test_size);
    if (buf == NULL) {
        ts_log(ERROR, "Failed to allocate test buffer");
        gaps_terminate();
        return NULL;
    }

    while (gaps_running()) {
        rd_len = pirate_read(REQ_TO_ECHO_RD, buf, uart_test->max_test_size);
        if (rd_len < 0) {
            gaps_terminate();
            continue;
        }

        wr_len = pirate_write(ECHO_TO_REQ_WR, buf, rd_len);
        if (wr_len != rd_len) {
            gaps_terminate();
            continue;
        }
    }

    free(buf);
    return NULL;
}


int main(int argc, char*argv[]) {
    uart_test_t uart_test = {
        .verbosity = VERBOSITY_NONE,
        .max_test_size = DEFAULT_MAX_TEST_SIZE,

        .app = {
            .threads = {
                THREAD_ADD(req_thread, &uart_test, "request_thread"),
                THREAD_ADD(echo_thread, &uart_test, "echo_thread"),
                THREAD_END
            },

            .ch = {
                GAPS_CHANNEL(REQ_TO_ECHO_WR, O_WRONLY, REQ_TO_ECHO_WR_CFG,
                            "R->E WR"),
                GAPS_CHANNEL(REQ_TO_ECHO_RD, O_RDONLY, REQ_TO_ECHO_RD_CFG,
                            "R->E RD"),
                GAPS_CHANNEL(ECHO_TO_REQ_WR, O_WRONLY, ECHO_TO_REQ_WR_CFG,
                            "E->R WR"),
                GAPS_CHANNEL(ECHO_TO_REQ_RD, O_RDONLY, ECHO_TO_REQ_RD_CFG,
                            "E->R RD")
            }
        }
    };

    parse_args(argc, argv, &uart_test);

    if (gaps_app_run(&uart_test.app) != 0) {
        ts_log(ERROR, "Failed to run UART test components");
        return -1;
    }

    return gaps_app_wait_exit(&uart_test.app);
}
