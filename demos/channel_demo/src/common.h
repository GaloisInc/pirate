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

#ifndef __GAPS_CHANNEL_DEMO_COMMON_H
#define __GAPS_CHANNEL_DEMO_COMMON_H

#include <argp.h>
#include <stdint.h>

#define OPT_DELIM ","

/* Verbosity levels */
typedef enum {
    VERBOSITY_NONE = 0,
    VERBOSITY_MIN  = 1,
    VERBOSITY_MAX  = 2
} verbosity_t;

/* Test data pattern */
typedef enum {
    ZEROS,
    ONES,
    INCR,
    BIN
} data_pattern_t;

/* Test data parameters */
#define DEFAULT_TEST_PATTERN INCR
#define DEFAULT_TEST_SIZE    100

typedef struct {
    const char *name;               /* Data description */
    data_pattern_t pattern;         /* Test data pattern */
    struct {
        uint32_t start;             /* Start length */
        uint32_t stop;              /* Stop length (exclusive) */
        uint32_t step;              /* Test length increment */
        uint32_t next;              /* Next test length */
    } len;
    uint32_t continuous;            /* Run in continuous mode */
    uint8_t *buf;                   /* Test buffer */
    const char *bin_input;          /* Path to optional input blob */
    const char *out_dir;            /* Output directory */
} test_data_t;

#define TEST_DATA_INIT(name_str) {      \
    .name      = name_str,              \
    .pattern   = DEFAULT_TEST_PATTERN,  \
    .len.start = DEFAULT_TEST_SIZE,     \
    .len.stop  = DEFAULT_TEST_SIZE + 1, \
    .len.step  = 1,                     \
    .len.next  = DEFAULT_TEST_SIZE,     \
    .buf       = NULL,                  \
    .bin_input = NULL,                  \
    .out_dir   = NULL                   \
}

/* Common test components */
typedef struct {
    uint32_t verbosity;
    test_data_t data;
} channel_test_t;

#define TEST_INIT(name_str) {              \
    .verbosity  = VERBOSITY_NONE,          \
    .data       = TEST_DATA_INIT(name_str) \
}

/* Common options */
#define COMMON_OPTIONS                                                         \
    { "pattern", 'p', "PAT",  0, "Test pattern (zeros,onces,incr)",  0 },      \
    { "cont",    'c', NULL,   0, "Operate in a continuous loop",     0 },      \
    { "length",  'l', "LEN",  0, "START,STOP,STEP",                  0 },      \
    { "input",   'i', "PATH", 0, "Binary input file path",           0 },      \
    { "save",    's', "DIR",  0, "Save test packets in a directory", 0 },      \
    { "verbose", 'v', NULL,   0, "Increase verbosity level",         0 },      \
    { "channel", 'C', "CH",   0, "<channel options>",                0 },      \
    { NULL,       0,  NULL,   0, GAPS_CHANNEL_OPTIONS,               0 }

int parse_common_options(int key, char *arg, channel_test_t *test,
                            struct argp_state *state, int ch_flags);

int test_data_parse_arg(char *str, test_data_t *td);
int test_data_init(test_data_t *td, verbosity_t v);
void test_data_term(test_data_t *td);
void test_data_get_next_len(test_data_t *td, uint32_t *len, uint32_t *done);
int bin_save(const char *prefix, const char *dir, const uint8_t *buf,
                uint32_t len);
int test_data_save(test_data_t *td, uint32_t len);

/* Logging */
typedef enum {
    INFO,
    WARN,
    ERROR
} log_level_t;

void log_msg(log_level_t l, const char *fmt, ...);
void print_hex(const char *msg, const uint8_t *buf, uint32_t len);

#endif /* __GAPS_CHANNEL_DEMO_COMMON_H */
