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

#include "greatest.h"
#include "primitives.h"
#include "ge_eth_test.h"

typedef enum {
    R_TO_E = 0,
    E_TO_R,
    CHANNEL_COUNT
} test_channels_t;

#define MAX_TEST_LEN      1400
#define TOTAL_TEST_LEN    (((MAX_TEST_LEN) * (MAX_TEST_LEN + 1)) >> 2)

#define GE_ETH_IP_ADDR    "127.0.0.1"
#define GE_ETH_IP_PORT    0x4745

static int open_gaps(int gd, channel_t type, int flags, const char *ip_addr,
    int port, channel_t *prev) {
    int rv = -1;

    *prev = pirate_get_channel_type(gd);

    rv = pirate_set_channel_type(gd, type);
    if (rv != 0) {
        return rv;
    }

    if (ip_addr != NULL) {
        rv = pirate_set_pathname(gd, ip_addr);
        if (rv != 0) {
            return rv;
        }
    }

    if (port != 0) {
        rv = pirate_set_port_number(gd, port);
        if (rv != 0) {
            return rv;
        }
    }

    return pirate_open(gd, flags);
}

static int close_gaps(int gd, int flags, channel_t prev) {
    int rv = pirate_close(gd, flags);
    if (rv != 0) {
        return rv;
    }

    rv = pirate_set_pathname(gd, NULL);
    pirate_set_channel_type(gd, prev);
    return rv;
}

static void fill(uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        buf[i] = i & 0xFF;
    }
}

static int validate(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        uint8_t val = buf[i];
        uint8_t exp = i & 0xFF;
        if (val != exp) {
            printf("Mismatch at %u: exp 0x%02X rx 0x%02X", i, exp, val);
            return -1;
        }
    }

    return 0;
}

TEST test_ge_eth_request() {
    uint8_t buf[MAX_TEST_LEN] = { 0 };
    int rv = -1;
    channel_t prev[CHANNEL_COUNT];

    rv = open_gaps(R_TO_E, GE_ETH, O_WRONLY, GE_ETH_IP_ADDR, GE_ETH_IP_PORT,
                    &prev[R_TO_E]);
    ASSERT_EQ_FMT(R_TO_E, rv, "%d");
    rv = open_gaps(E_TO_R, PIPE, O_RDONLY, NULL, 0, &prev[E_TO_R]);
    ASSERT_EQ_FMT(E_TO_R, rv, "%d");

    for (ssize_t test_len = 1; test_len <= MAX_TEST_LEN; test_len++) {
        fill(buf, test_len);

        ssize_t len = pirate_write(R_TO_E, buf, test_len);
        ASSERT_EQ_FMT(len, test_len, "%zd");

        len = pirate_read(E_TO_R, buf, len);
        ASSERT_EQ_FMT(len, test_len, "%zd");

        rv = validate(buf, test_len);
        ASSERT_EQ_FMT(0, rv, "%d");
    }

    rv = close_gaps(R_TO_E, O_WRONLY, prev[R_TO_E]);
    ASSERT_EQ_FMT(0, rv, "%d");
    rv = close_gaps(E_TO_R, O_RDONLY, prev[E_TO_R]);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}

TEST test_ge_eth_echo() {
    uint8_t buf[MAX_TEST_LEN] = { 0 };
    int rv = -1;
    channel_t prev[CHANNEL_COUNT];

    rv = open_gaps(R_TO_E, GE_ETH, O_RDONLY, GE_ETH_IP_ADDR, GE_ETH_IP_PORT,
        &prev[R_TO_E]);
    ASSERT_EQ_FMT(R_TO_E, rv, "%d");
    rv = open_gaps(E_TO_R, PIPE, O_WRONLY, NULL, 0, &prev[E_TO_R]);
    ASSERT_EQ_FMT(E_TO_R, rv, "%d");

    for (ssize_t test_len = 1; test_len <= MAX_TEST_LEN; test_len++) {
        ssize_t len = pirate_read(R_TO_E, buf, MAX_TEST_LEN);
        ASSERT_EQ_FMT(len, test_len, "%zd");

        len = pirate_write(E_TO_R, buf, len);
        ASSERT_EQ_FMT(len, test_len, "%zd");
    }

    rv = close_gaps(R_TO_E, O_RDONLY, prev[R_TO_E]);
    ASSERT_EQ_FMT(0, rv, "%d");
    rv = close_gaps(E_TO_R, O_WRONLY, prev[E_TO_R]);
    ASSERT_EQ_FMT(0, rv, "%d");

    PASS();
}

void *test_ge_eth_request_thread(__attribute__((unused)) void *unused) {
  return (void *)test_ge_eth_request();
}

void *test_ge_eth_echo_thread(__attribute__((unused)) void *unused) {
  return (void *)test_ge_eth_echo();
}

enum greatest_test_res test_communication_pthread_ge_eth()  {
    int rv = -1;
    pthread_t req_id, echo_id;
    void *req_sts, *echo_sts;

    rv = pthread_create(&req_id, NULL, test_ge_eth_request_thread, NULL);
    if (rv != 0) {
        FAILm(strerror(rv));
    }

    rv = pthread_create(&echo_id, NULL, test_ge_eth_echo_thread, NULL);
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

    PASS();
}
