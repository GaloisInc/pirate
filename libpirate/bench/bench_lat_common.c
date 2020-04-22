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

#include "libpirate.h"

extern int test_gd1, test_gd2, sync_gd;
uint64_t nbytes;
size_t message_len;
extern char message[80];
extern unsigned char *buffer1, *buffer2;

static int bench_lat_open(int num, char *param_str, pirate_channel_param_t *param, int flags) {
    int err, fd, rv;
    size_t bufsize;
    
    bufsize = 8 * message_len;

    switch (param->channel_type) {
        case SHMEM:
            if ((bufsize > DEFAULT_SMEM_BUF_LEN) && (param->channel.shmem.buffer_size == 0)) {
                param->channel.shmem.buffer_size = bufsize;
            }
            break;
        case UNIX_SOCKET:
            if ((bufsize > 212992) && (param->channel.unix_socket.buffer_size == 0)) {
                param->channel.unix_socket.buffer_size = bufsize;
            }
            break;
        case UDP_SHMEM:
            if (param->channel.udp_shmem.packet_size == 0) {
                param->channel.udp_shmem.packet_size = message_len;
            }
            break;
        default:
            break;
    }

    rv = pirate_open_param(param, flags);
    if (rv < 0) {
        snprintf(message, sizeof(message), "Unable to open test channel %d \"%s\"", num, param_str);
        perror(message);
        if (param->channel_type == UNIX_SOCKET) {
            fprintf(stderr, "Check /proc/sys/net/core/wmem_max\n");
        }
        return rv;
    }
    err = errno;
    fd = pirate_get_fd(rv);
    errno = err;
    switch (param->channel_type) {
        case PIPE:
            if (fcntl(fd, F_SETPIPE_SZ, bufsize) < 0) {
                snprintf(message, sizeof(message),
                    "Unable to set F_SETPIPE_SZ option on test channel %d \"%s\"",
                    num, param_str);
                perror(message);
                fprintf(stderr, "Check /proc/sys/fs/pipe-max-size\n");
                return -1;
            }
            break;
        case TCP_SOCKET: {
            struct linger socket_reset;
            int enable = 1;
            socket_reset.l_onoff = 1;
            socket_reset.l_linger = 0;
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &socket_reset,
                        sizeof(socket_reset)) < 0) {
                snprintf(message, sizeof(message),
                    "Unable to set SO_LINGER option on test channel %d \"%s\"",
                    num, param_str);
                perror(message);
                return -1;
            }
            // TCP_NODELAY is unique to latency tests
            // Do not enable TCP_NODELAY on throughput tests
            if (setsockopt(fd,  IPPROTO_TCP, TCP_NODELAY, &enable,
                        sizeof(enable)) < 0) {
                snprintf(message, sizeof(message),
                    "Unable to set TCP_NODELAY option on test channel %d \"%s\"",
                    num, param_str);
                perror(message);
                return -1;
            }
            break;
        }
        default:
            break;
    }
    
    return rv;
}

int bench_lat_setup(char *argv[], int test_flag1, int test_flag2, int sync_flags) {
    char *endptr;
    pirate_channel_param_t param1, param2;

    if (strstr(argv[3], "tcp_socket,") == NULL) {
        fprintf(stderr, "Sync channel \"%s\" must be a tcp socket\n", argv[2]);
        return 1;
    }

    if (pirate_parse_channel_param(argv[1], &param1)) {
        fprintf(stderr, "Unable to parse test channel 1 \"%s\"\n", argv[1]);
        return 1;
    }

    if (pirate_parse_channel_param(argv[2], &param2)) {
        fprintf(stderr, "Unable to parse test channel 2 \"%s\"\n", argv[2]);
        return 1;
    }

    if (param1.channel_type != param2.channel_type) {
        fprintf(stderr, "Test channels \"%s\" and \"%s\" are of different type\n", argv[1], argv[2]);
        return 1;
    }

    message_len = strtol(argv[4], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Unable to parse message length \"%s\"\n", argv[4]);
        return 1;
    }

    nbytes = strtol(argv[5], &endptr, 10);
    if (*endptr != '\0') {
        snprintf(message, sizeof(message), "Unable to parse number of bytes \"%s\"", argv[5]);
        perror(message);
        return 1;
    }

    test_gd1 = bench_lat_open(1, argv[1], &param1, test_flag1);
    if (test_gd1 < 0) {
        return 1;
    }

    test_gd2 = bench_lat_open(2, argv[2], &param2, test_flag2);
    if (test_gd2 < 0) {
        return 1;
    }

    sync_gd = pirate_open_parse(argv[3], sync_flags);
    if (sync_gd < 0) {
        snprintf(message, sizeof(message), "Unable to open sync channel \"%s\"", argv[3]);
        perror(message);
        return 1;
    }

    // truncate nbytes to be divisible by message_len
    nbytes = message_len * (nbytes / message_len);

    buffer1 = malloc(nbytes);
    if (buffer1 == NULL) {
        fprintf(stderr, "Failed to allocate buffer 1 of %zu bytes\n", nbytes);
        return 1;
    }

    buffer2 = malloc(nbytes);
    if (buffer2 == NULL) {
        fprintf(stderr, "Failed to allocate buffer 2 of %zu bytes\n", nbytes);
        return 1;
    }

    return 0;
}

void bench_lat_close(char *argv[]) {
    if (buffer1 != NULL) {
        free(buffer1);
    }
    if (buffer2 != NULL) {
        free(buffer2);
    }    
    if ((test_gd1 >= 0) && (pirate_close(test_gd1) < 0)) {
        snprintf(message, sizeof(message), "Unable to close test channel 1 %s", argv[1]);
        perror(message);
    }
    if ((test_gd2 >= 0) && (pirate_close(test_gd2) < 0)) {
        snprintf(message, sizeof(message), "Unable to close test channel 2 %s", argv[2]);
        perror(message);
    }    
    if ((sync_gd >= 0) && (pirate_close(sync_gd) < 0)) {
        snprintf(message, sizeof(message), "Unable to close sync channel %s", argv[3]);
        perror(message);
    }
}