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

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include "greatest.h"
#include "primitives.h"
#include "serial_test.h"

typedef enum {
    REQ_TO_ECHO_WR = 0,
    REQ_TO_ECHO_RD = 1,
    ECHO_TO_REQ_WR = 2,
    ECHO_TO_REQ_RD = 3,
    CHANNEL_COUNT
} test_channels_t;

#define REQ_TO_ECHO_WR_PATH     "/dev/ttyUSB0"
#define REQ_TO_ECHO_RD_PATH     "/dev/ttyUSB1"
#define ECHO_TO_REQ_WR_PATH     "/dev/ttyUSB2"
#define ECHO_TO_REQ_RD_PATH     "/dev/ttyUSB3"

typedef enum {
    COUNT = 0,
    ZEROS,
    ONES,
    ALT,
    PATTERN_COUNT
} pattern_type_t;

#define ALT_PATTERN             0xAA

#define MAX_TEST_LEN            (1 << 10)
#define TOTAL_TEST_LEN          (((MAX_TEST_LEN << 1) - 1) * PATTERN_COUNT)

static int open_serial(int gd, int flags, const char *path, channel_t *prev) {
    int rv = -1;

    *prev = pirate_get_channel_type(gd);

    rv = pirate_set_channel_type(gd, SERIAL);
    if (rv != 0) {
        return rv;
    }

    rv = pirate_set_pathname(gd, path);
    if (rv != 0) {
        return rv;
    }

    return pirate_open(gd, flags);
}

static int close_serial(int gd, int flags, channel_t prev) {
    int rv = pirate_close(gd, flags);
    if (rv != 0) {
        return rv;
    }

    rv = pirate_set_pathname(gd, NULL);
    pirate_set_channel_type(gd, prev);
    return rv;
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


TEST test_serial_request() {
    int rv = -1;
    uint8_t *buf = NULL;

    buf = (uint8_t *)malloc(MAX_TEST_LEN);
    ASSERT(buf != NULL);

    ssize_t test_len = 1;
    pattern_type_t pattern = COUNT;
    while (test_len <= MAX_TEST_LEN) {
        fill(buf, test_len, pattern);

        ssize_t len = pirate_write(REQ_TO_ECHO_WR, buf, test_len);
        ASSERT_EQ_FMT(len, test_len, "%zd");

        uint8_t *rd_buf = buf;
        do {
            ssize_t rd_len = pirate_read(ECHO_TO_REQ_RD, rd_buf, len);
            ASSERT(rd_len != -1);
            rd_buf = rd_buf + rd_len;
            len -= rd_len;
        } while (len > 0);

        rv = validate(buf, test_len, pattern);
        ASSERT_EQ_FMT(0, rv, "%d");

        if (++pattern == PATTERN_COUNT) {
            pattern = COUNT;
            test_len <<= 1;
        }
    }

    free(buf);
    buf = NULL;

    PASS();
}

TEST test_serial_echo() {
    uint8_t *buf = (uint8_t *)malloc(MAX_TEST_LEN);
    ASSERT(buf != NULL);

    ssize_t remain_len = TOTAL_TEST_LEN;
    do {
        ssize_t rd_len = pirate_read(REQ_TO_ECHO_RD, buf, MAX_TEST_LEN);
        ASSERT(rd_len > 0);

        ssize_t wr_len = pirate_write(ECHO_TO_REQ_WR, buf, rd_len);
        ASSERT_EQ_FMT(rd_len, wr_len, "%zd");

        remain_len -= rd_len;
    } while (remain_len > 0);

    free(buf);
    buf = NULL;

    PASS();
}


void *test_serial_request_thread(__attribute__((unused)) void *unused) {
  return (void *)test_serial_request();
}

void *test_serial_echo_thread(__attribute__((unused)) void *unused) {
  return (void *)test_serial_echo();
}

enum greatest_test_res test_communication_pthread_serial() {
    int rv = -1;
    channel_t prev[CHANNEL_COUNT];
    pthread_t req_id, echo_id;
    void *req_sts, *echo_sts;

    if (access(REQ_TO_ECHO_WR_PATH, F_OK) == -1) {
        SKIPm("Device " REQ_TO_ECHO_WR_PATH " not found");
    }
    if (access(ECHO_TO_REQ_RD_PATH, F_OK) == -1) {
        SKIPm("Device " ECHO_TO_REQ_RD_PATH " not found");
    }
    if (access(REQ_TO_ECHO_RD_PATH, F_OK) == -1) {
        SKIPm("Device " REQ_TO_ECHO_RD_PATH " not found");
    }
    if (access(ECHO_TO_REQ_WR_PATH, F_OK) == -1) {
        SKIPm("Device " ECHO_TO_REQ_WR_PATH " not found");
    }

    rv = open_serial(REQ_TO_ECHO_WR, O_WRONLY, REQ_TO_ECHO_WR_PATH, &prev[REQ_TO_ECHO_WR]);
    ASSERT_EQ_FMT(REQ_TO_ECHO_WR, rv, "%d");
    rv = open_serial(ECHO_TO_REQ_RD, O_RDONLY, ECHO_TO_REQ_RD_PATH, &prev[ECHO_TO_REQ_RD]);
    ASSERT_EQ_FMT(ECHO_TO_REQ_RD, rv, "%d");
    rv = open_serial(REQ_TO_ECHO_RD, O_RDONLY, REQ_TO_ECHO_RD_PATH, &prev[REQ_TO_ECHO_RD]);
    ASSERT_EQ_FMT(REQ_TO_ECHO_RD, rv, "%d");
    rv = open_serial(ECHO_TO_REQ_WR, O_WRONLY, ECHO_TO_REQ_WR_PATH, &prev[ECHO_TO_REQ_WR]);
    ASSERT_EQ_FMT(ECHO_TO_REQ_WR, rv, "%d");

    rv = pthread_create(&req_id, NULL, test_serial_request_thread, NULL);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    rv = pthread_create(&echo_id, NULL, test_serial_echo_thread, NULL);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    rv = pthread_join(req_id, &req_sts);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    rv = pthread_join(echo_id, &echo_sts);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    if (((greatest_test_res)req_sts) == GREATEST_TEST_RES_FAIL) {
        if (GREATEST_ABORT_ON_FAIL()) {
            abort();
        }
        return GREATEST_TEST_RES_FAIL;
    }

    if (((greatest_test_res)echo_sts) == GREATEST_TEST_RES_FAIL) {
        if (GREATEST_ABORT_ON_FAIL()) {
            abort();
        }
        return GREATEST_TEST_RES_FAIL;
    }

    rv = close_serial(REQ_TO_ECHO_WR, O_WRONLY, prev[REQ_TO_ECHO_WR]);
    ASSERT_EQ_FMT(0, rv, "%d");
    rv = close_serial(ECHO_TO_REQ_RD, O_RDONLY, prev[ECHO_TO_REQ_RD]);
    ASSERT_EQ_FMT(0, rv, "%d");
    rv = close_serial(REQ_TO_ECHO_RD, O_RDONLY, prev[REQ_TO_ECHO_RD]);
    ASSERT_EQ_FMT(0, rv, "%d");
    rv = close_serial(ECHO_TO_REQ_WR, O_WRONLY, prev[ECHO_TO_REQ_WR]);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}
