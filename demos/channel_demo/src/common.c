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
#include "common.h"

#define TIMESTAMP_STR_LEN 64
#ifndef DEMO_VERSION
#define DEMO_VERSION ""
#endif
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
    char *endptr = NULL;

    /* Start */
    if ((ptr = strtok(str, OPT_DELIM)) == NULL) {
        return -1;
    }
    td->len.next = td->len.start = strtol(ptr, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse numeric value from '%s'\n", ptr);
    }

    /* End */
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        td->len.step = 1;
        td->len.stop = td->len.start + td->len.step;
        return 0;
    }
    td->len.stop = strtol(ptr, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse numeric value from '%s'\n", ptr);
    }

    /* Step */
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        td->len.step = 1;
        return 0;
    }
    td->len.step = strtol(ptr, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse numeric value from '%s'\n", ptr);
    }

    return 0;
}

int perf_parse_arg(char *str, test_data_t *td) {
    char *ptr = NULL;
    char *endptr = NULL;

    /* Message length */
    if ((ptr = strtok(str, OPT_DELIM)) == NULL) {
        return -1;
    }
    td->perf.len = strtol(ptr, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse numeric value from '%s'\n", ptr);
    }

    if (td->perf.len < sizeof(msg_index_t)) {
        fprintf(stderr, "Message length must be at least %zu bytes\n", 
            sizeof(msg_index_t));
        return -1;
    }

    /* Message count */
    if ((ptr = strtok(NULL, OPT_DELIM)) == NULL) {
        return -1;
    }
    td->perf.count = strtol(ptr, &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse numeric value from '%s'\n", ptr);
    }

    td->perf.enabled = 1;
    return 0;
}


int parse_common_options(int key, char *arg, channel_test_t *test,
                            struct argp_state *state) {
    int rv = 1;
    char* endptr = NULL;

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

    case 'P':
        if (perf_parse_arg(arg, &test->data) != 0) {
            argp_failure(state, 1, 0, 
                            "Failed to parse perfomance test opt '%s'", arg);
        }
        break;

    case 'd':
        test->data.delay_ns = strtoull(arg, &endptr, 10);
        if (*endptr != '\0') {
            argp_error(state, "Unable to parse numeric value from '%s'\n", arg);
        }
        break;

    case 'C':
        test->conf = arg;
        break;

    default:            /* Not a common option */
        rv = 0;
    }

    return rv;
}

int test_data_init(test_data_t *td, verbosity_t v) {
    if (td->perf.enabled) {
        const uint32_t data_len = td->perf.len - sizeof(msg_index_t);
        
        td->buf = (uint8_t *) calloc(1, td->perf.len);
        if (td->buf == NULL) {
            log_msg(ERROR, "Failed to allocate %u bytes", td->perf.len);
            return -1;
        }

        /* Incremental pattern follows the packet sequence */
        for (uint32_t i = 0; i < data_len; ++i) {
            td->buf[sizeof(msg_index_t) + i] = i & 0xFF;
        }

        if (v >= VERBOSITY_MIN) {
            log_msg(INFO, "Performance test:");
            log_msg(INFO, "Message length %u, Count %u", td->perf.len,
                    td->perf.count);
        }
    } else if (td->bin_input != NULL) {
        /* Use provided binary file as input */
        FILE *f_in = NULL;
        struct stat st;

        memset(&st, 0, sizeof(struct stat));
        if (stat(td->bin_input, &st) != 0) {
            log_msg(ERROR, "Failed to stat %s", td->bin_input);
            return -1;
        }

        td->buf = (uint8_t *)calloc(1, st.st_size);
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

        td->buf = (uint8_t *)calloc(1, buf_len);
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

        if (v >= VERBOSITY_MIN) {
            log_msg(INFO, "Test data pattern: %s", pattern_str(td->pattern));
            log_msg(INFO, "Start %u, stop %u, step %u", td->len.start, td->len.stop,
                    td->len.step);
        }
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

