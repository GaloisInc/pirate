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

#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bench_thr.h"

static struct argp_option options[] = {
    { "channel",     'c', "CONFIG", 0, "Test channel configuration",    0 },
    { "sync1",       's', "CONFIG", 0, "Sync channel 1 configuration",  0 },
    { "sync2",       'S', "CONFIG", 0, "Sync channel 2 configuration",  0 },
    { "nbytes",      'n', "BYTES",  0, "Number of bytes to receive",    0 },
    { "message_len", 'm', "BYTES",  0, "Transfer message size",         0 },
    { "validate",    'v', NULL,     0, "Validate received data",        0 },
    { "tx_delay",    'd', "NSEC",   0, "Inter-message delay",           0 },
    { "rx_timeout",  'w', "SEC",    0, "Message receive timeout",       0 },
    { NULL,           0,  NULL,     0, GAPS_CHANNEL_OPTIONS,            2 },
    { NULL,           0,  NULL,     0, 0,                               0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    bench_thr_t *bench = (bench_thr_t *) state->input;
    char* endptr = NULL;

    switch (key) {

    case 'c':
        bench->test_ch.config = arg;
        break;

    case 's':
        bench->sync_ch1.config = arg;
        break;

    case 'S':
        bench->sync_ch2.config = arg;
        break;

    case 'n':
        bench->nbytes = strtol(arg, &endptr, 10);
        if (*endptr != '\0') {
            argp_error(state, "Unable to parse numeric value from \"%s\"\n", arg);
        }
        break;

    case 'm':
        bench->message_len = strtol(arg, &endptr, 10);
        if (*endptr != '\0') {
            argp_error(state, "Unable to parse numeric value from \"%s\"\n", arg);
        }
        break;

    case 'v':
        bench->validate = 1;
        break;

    case 'd':
        bench->tx_delay_ns = strtoull(arg, &endptr, 10);
        if (*endptr != '\0') {
            argp_error(state, "Unable to parse numeric value from \"%s\"\n", arg);
        }
        break;

    case 'w':
        bench->rx_timeout_s = strtol(arg, &endptr, 10);
        if (*endptr != '\0') {
            argp_error(state, "Unable to parse numeric value from \"%s\"\n", arg);
        }
        break;

    case ARGP_KEY_END:
        if (bench->test_ch.config == NULL) {
            argp_error(state, "Test channel configuration is not specified");
        }

        if (bench->sync_ch1.config == NULL) {
            argp_error(state, "Sync channel 1 configuration is not specified");
        }

        if (bench->sync_ch2.config == NULL) {
            argp_error(state, "Sync channel 2 configuration is not specified");
        }

        if (strstr(bench->sync_ch1.config, "tcp_socket,") == NULL) {
            argp_error(state,"Sync channel 1 '%s' must be a tcp_socket",
                        bench->sync_ch1.config);
        }

        if (strstr(bench->sync_ch2.config, "tcp_socket,") == NULL) {
            argp_error(state,"Sync channel 2 '%s' must be a tcp_socket",
                        bench->sync_ch2.config);
        }
        break;

    default:
        break;
    }

    return 0;
}

void parse_args(int argc, char *argv[], bench_thr_t *bench) {
    /* Default parameters */
    bench->test_ch.config  = NULL;
    bench->test_ch.gd      = -1;
    bench->sync_ch1.config = NULL;
    bench->sync_ch1.gd     = -1;
    bench->sync_ch2.config = NULL;
    bench->sync_ch2.gd     = -1;
    bench->nbytes          = 1024;
    bench->message_len     = 128;
    bench->validate        = 0;
    bench->tx_delay_ns     = 0;
    bench->rx_timeout_s    = 2;

    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = NULL,
        .doc = "PIRATE throughput benchmark",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, bench);
}

static int bench_thr_open(bench_thr_t *bench, int flags) {
    pirate_channel_param_t param;
    size_t bufsize = 8 * bench->message_len;
    int err, fd;

    if (pirate_parse_channel_param(bench->test_ch.config, &param)) {
        fprintf(stderr, "Unable to parse test channel \"%s\"\n",
            bench->test_ch.config);
        return -1;
    }

    switch (param.channel_type) {
        case SHMEM:
            if ((bufsize > PIRATE_DEFAULT_SMEM_BUF_LEN) && (param.channel.shmem.buffer_size == 0)) {
                param.channel.shmem.buffer_size = MIN(bufsize, 524288);
            }
            break;
        case UNIX_SOCKET:
            if ((bufsize > 212992) && (param.channel.unix_socket.buffer_size == 0)) {
                param.channel.unix_socket.buffer_size = bufsize;
            }
            break;
        case UDP_SHMEM:
            if (param.channel.udp_shmem.packet_size == 0) {
                param.channel.udp_shmem.packet_size = MAX(bench->message_len, 64);
            }
            break;
        default:
            break;
    }

    bench->test_ch.gd = pirate_open_param(&param, flags);
    if (bench->test_ch.gd < 0) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to open test channel \"%s\"",
                    bench->test_ch.config);
        perror(bench->err_msg);
        if (param.channel_type == UNIX_SOCKET) {
            fprintf(stderr, "Check /proc/sys/net/core/wmem_max\n");
        }
        return bench->test_ch.gd;
    }

    err = errno;
    fd = bench->test_ch.gd;
    errno = err;
    switch (param.channel_type) {
        case PIPE:
            if (fcntl(fd, F_SETPIPE_SZ, bufsize) < 0) {
                snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to set F_SETPIPE_SZ option on test channel \"%s\"",
                    bench->test_ch.config);
                perror(bench->err_msg);
                fprintf(stderr, "Check /proc/sys/fs/pipe-max-size\n");
                return -1;
            }
            break;
        case UDP_SOCKET:
        case GE_ETH: {
            struct timeval tv;
            tv.tv_sec = bench->rx_timeout_s;
            tv.tv_usec = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                        sizeof(tv)) < 0) {
                snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to set SO_RCVTIMEO option on test channel \"%s\"",
                    bench->test_ch.config);
                perror(bench->err_msg);
                return -1;
            }
            break;
        }
        case TCP_SOCKET: {
            struct linger socket_reset;
            socket_reset.l_onoff = 1;
            socket_reset.l_linger = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &socket_reset,
                        sizeof(socket_reset)) < 0) {
                snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to set SO_LINGER option on test channel \"%s\"",
                    bench->test_ch.config);
                perror(bench->err_msg);
                return -1;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

int bench_thr_setup(bench_thr_t *bench, int test_flags, int sync_flags1, int sync_flags2) {
    /* Open the synchronization channels */
    bench->sync_ch1.gd = pirate_open_parse(bench->sync_ch1.config, sync_flags1);
    if (bench->sync_ch1.gd < 0) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to open sync channel 1 \"%s\"",
                    bench->sync_ch1.config);
        perror(bench->err_msg);
        return -1;
    }

    bench->sync_ch2.gd = pirate_open_parse(bench->sync_ch2.config, sync_flags2);
    if (bench->sync_ch2.gd < 0) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to open sync channel 2 \"%s\"",
                    bench->sync_ch2.config);
        perror(bench->err_msg);
        return -1;
    }

    /* Open the test channel */
    if (bench_thr_open(bench, test_flags) != 0) {
        return -1;
    }

    /* Truncate nbytes to be divisible by message_len */
    bench->nbytes = bench->message_len * (bench->nbytes / bench->message_len);
    const uint32_t iter = bench->nbytes / bench->message_len;

    bench->buffer = calloc(bench->nbytes, 1);
    if (bench->buffer == NULL) {
        fprintf(stderr, "Failed to allocate buffer of %zu bytes\n",
                    bench->nbytes);
        return -1;
    }

    bench->bitvector = calloc(iter / 8 + 1, 1);
    if (bench->buffer == NULL) {
        fprintf(stderr, "Failed to allocate bitvector of %d bytes\n",
                    iter / 8 + 1);
        return -1;
    }

    return 0;
}

void bench_thr_close(bench_thr_t *bench) {
    if (bench->buffer != NULL) {
        free(bench->buffer);
        bench->buffer = NULL;
    }

    if (bench->bitvector != NULL) {
        free(bench->bitvector);
        bench->buffer = NULL;
    }

    if ((bench->test_ch.gd >= 0) && (pirate_close(bench->test_ch.gd) < 0)) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to close test channel %s", bench->test_ch.config);
        perror(bench->err_msg);
    }

    if ((bench->sync_ch1.gd >= 0) && (pirate_close(bench->sync_ch1.gd) < 0)) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to close sync channel 1 %s", bench->sync_ch1.config);
        perror(bench->err_msg);
    }

    if ((bench->sync_ch2.gd >= 0) && (pirate_close(bench->sync_ch2.gd) < 0)) {
        snprintf(bench->err_msg, sizeof(bench->err_msg),
                    "Unable to close sync channel 2 %s", bench->sync_ch2.config);
        perror(bench->err_msg);
    }
}
