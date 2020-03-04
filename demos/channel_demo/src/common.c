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
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "primitives.h"
#include "common.h"

#define TIMESTAMP_STR_LEN 64
const char *argp_program_version = DEMO_VERSION;

static const char *pattern_str(data_pattern_t p) {
    switch (p) {
        case ZEROS:
            return "zeros";
        case ONES:
            return "ones";
        case INCR:
            return "incrementing bytes";
        case BIN:
            return "binary blob";
        default:
            return "invalid";
    }

    return NULL;
}

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

static int parse_channel_opt(char *str, int flags) {
    int rv;
    pirate_channel_param_t param;
    channel_t channel = pirate_parse_channel_param(GAPS_CHANNEL, flags, str,
                                                    &param);
    if (channel == INVALID) {
        log_msg(ERROR, "failed to parse channel options '%s'", str);
        return -1;
    }

    rv = pirate_set_channel_param(channel, GAPS_CHANNEL, flags, &param);
    if (rv != 0) {
        log_msg(ERROR, "failed to set channel options '%s'", str);
        return -1;
    }

    if (pirate_open(GAPS_CHANNEL, flags) != GAPS_CHANNEL) {
        log_msg(ERROR, "Failed to open GAPS channel");
        return -1;
    }

    return 0;
}


static int str_to_data_pattern(const char *str, data_pattern_t *pattern) {
    if (strcmp(str, "zeros") == 0) {
        *pattern = ZEROS;
    } else if (strcmp(str, "ones") == 0) {
        *pattern = ONES;
    } else if (strcmp(str, "incr") == 0) {
        *pattern = INCR;
    } else if (strcmp(str, "bin") == 0) {
        *pattern = BIN;
    } else {
        return -1;
    }

    return 0;
}

int test_data_parse_arg(char *str, test_data_t *td) {
    char *ptr = NULL;

    /* Start */
    if ((ptr = strtok(str, OPT_DELIM)) == NULL) {
        return -1;
    }
    td->len.next = td->len.start = strtol(ptr, NULL, 10);

    /* End */
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        td->len.step = 1;
        td->len.stop = td->len.start + td->len.step;
        return 0;
    }
    td->len.stop = strtol(ptr, NULL, 10);

    /* Step */
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        td->len.step = 1;
        return 0;
    }
    td->len.step = strtol(ptr, NULL, 10);

    return 0;
}


int parse_common_options(int key, char *arg, channel_test_t *test,
                            struct argp_state *state, int ch_flags) {
    int rv = 1;

    switch (key) {

    case 'p':
        if (str_to_data_pattern(arg, &test->data.pattern) != 0) {
            argp_failure(state, 1, 0, "Invalid data pattern '%s'", arg);
        }
        break;

    case 'c':
        test->data.continuous = 1;
        break;

    case 'l':
        if (test_data_parse_arg(arg, &test->data) != 0) {
            argp_failure(state, 1, 0, "Invalid data length option '%s'", arg);
        }
        break;

    case 'i':
        test->data.bin_input = arg;
        break;

    case 's':
        test->data.out_dir = arg;
        break;

    case 'v':
        if (test->verbosity < VERBOSITY_MAX) {
            test->verbosity++;
        }
        break;

    case 'C':
        if (parse_channel_opt(arg, ch_flags) != 0) {
            argp_failure(state, 1, 0, "Failed to initialize channel %s", arg);
        }
        break;

    default:            /* Not a common option */
        rv = 0;
    }

    return rv;
}

int test_data_init(test_data_t *td, verbosity_t v) {
    if (td->bin_input != NULL) {
        /* Use provided binary file as input */
        FILE *f_in = NULL;
        struct stat st = { 0 };
        if (stat(td->bin_input, &st) != 0) {
            log_msg(ERROR, "Failed to stat %s", td->bin_input);
            return -1;
        }

        td->buf = (uint8_t *)malloc(st.st_size);
        if (td->buf == NULL) {
            log_msg(ERROR, "Failed to allocate %u bytes", st.st_size);
            return -1;
        }

        td->len.step = 1;
        td->len.next = td->len.start = st.st_size;
        td->len.stop = td->len.start + td->len.step;

        if ((f_in = fopen(td->bin_input, "rb")) == NULL) {
            log_msg(ERROR, "Failed to open %s", td->bin_input);
            return -1;
        }

        if (fread(td->buf, st.st_size, 1, f_in) != 1) {
            log_msg(ERROR, "Failed to read binary test data");
            return -1;
        }

        fclose(f_in);

        if (v >= VERBOSITY_MIN) {
            log_msg(INFO, "Using %u bytes from %s", st.st_size, td->bin_input);
            if (v >= VERBOSITY_MAX) {
                print_hex("Test binary data", td->buf, st.st_size);
            }
        }

        return 0;
    } else {
        const uint32_t buf_len = td->len.stop - 1;

        td->buf = (uint8_t *)malloc(buf_len);
        if (td->buf == NULL) {
            log_msg(ERROR, "Failed to allocate %u bytes", buf_len);
            return -1;
        }

        switch (td->pattern) {
            case ZEROS:
                memset(td->buf, 0x00, buf_len);
                break;

            case ONES:
                memset(td->buf, 0xFF, buf_len);
                break;

            case INCR:
                for (uint32_t i = 0; i < buf_len; ++i) {
                    td->buf[i] = i & 0xFF;
                }
                break;

            case BIN:
            default:
                log_msg(ERROR, "Invalid data pattern");
                return -1;
        }
    }

    if (v >= VERBOSITY_MIN) {
        log_msg(INFO, "Test data pattern: %s", pattern_str(td->pattern));
        log_msg(INFO, "Start %u, stop %u, step %u", td->len.start, td->len.stop,
                td->len.step);
    }

    return 0;
}


void test_data_term(test_data_t *td) {
    if (td->buf != NULL) {
        free(td->buf);
        td->buf = NULL;
    }
}

void test_data_get_next_len(test_data_t *td, uint32_t *len, uint32_t *done) {
    *len = td->len.next;
    td->len.next += td->len.step;

    if (td->len.next < td->len.stop) {
        *done = 0;
    } else {
        td->len.next = td->len.start;
        *done = !td->continuous;
    }
}

int bin_save(const char *prefix, const char *dir, const uint8_t *buf,
                uint32_t len) {
    FILE *f_out = NULL;
    char ts[TIMESTAMP_STR_LEN];
    char path[PATH_MAX];

    if (dir == NULL) {
        dir = ".";
    }

    if (get_time(ts, "%Y_%m_%d_%H.%M.%S") != 0) {
        log_msg(ERROR, "Failed to get the current time");
        return -1;
    }

    snprintf(path, sizeof(path) - 1, "%s/%s_%s.bin", dir, prefix, ts);

    if ((f_out = fopen(path, "wb")) == NULL) {
        log_msg(ERROR, "Failed to open output file %s", path);
        return -1;
    }

    if (fwrite(buf, len, 1, f_out) != 1) {
        log_msg(ERROR, "Failed to save binary content");
        return -1;
    }

    fclose(f_out);

    return 0;
}

int test_data_save(test_data_t *td, uint32_t len) {
    if (td->out_dir == NULL) {
        return 0;
    }

    return bin_save(td->name, td->out_dir, td->buf, len);
}

void log_msg(log_level_t l, const char *fmt, ...) {
    FILE *stream = NULL;
    char *msg = NULL;
    size_t len = 0;
    va_list args;
    char ts[TIMESTAMP_STR_LEN];

    const char *evt_str = l == INFO ? "INFO" : l == WARN ?  "WARN" : "ERR ";

    if (get_time(ts, "%Y-%m-%d %H:%M:%S") != 0) {
        fprintf(stderr, "Failed to get the current time");
        return;
    }

    if ((stream = open_memstream(&msg, &len)) == NULL) {
        fprintf(stderr, "Failed to create output stream");
        return;
    }

    fprintf(stream, "[%s] %s ", ts, evt_str);
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    if ((l != INFO) && (errno != 0)) {
        fprintf(stream, " (%s)", strerror(errno));
    }

    fflush(stream);
    fclose(stream);
    puts(msg);
    free(msg);
}

void print_hex(const char *msg, const uint8_t *buf, uint32_t len) {
    const uint32_t row_len = 1 << 5;
    fprintf(stdout, "%s", msg);

    for (uint32_t i = 0; i < len; ++i) {
        if ((i & (row_len - 1)) == 0) {
            fprintf(stdout, "\n\t0x%04X:", i);
        }

        if ((i & 3) == 0) {
            fprintf(stdout, " ");
        }
        fprintf(stdout, "%02X", buf[i]);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

