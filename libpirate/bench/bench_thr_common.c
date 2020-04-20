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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpirate.h"

extern int test_gd, sync_gd;
extern size_t message_len, nbytes;
extern char message[80];
extern unsigned char* buffer;

int bench_thr_setup(char *argv[], int test_flags, int sync_flags) {
    char* endptr;    
    if (strstr(argv[2], "tcp_socket,") == NULL) {
        printf("Sync channel %s must be a tcp socket\n", argv[2]);
        return 1;
    }

    test_gd = pirate_open_parse(argv[1], test_flags);
    if (test_gd < 0) {
        snprintf(message, sizeof(message), "Unable to open test channel %s", argv[1]);
        perror(message);
        return 1;
    }

    sync_gd = pirate_open_parse(argv[2], sync_flags);
    if (sync_gd < 0) {
        snprintf(message, sizeof(message), "Unable to open sync channel %s", argv[2]);
        perror(message);
        return 1;
    }

    message_len = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0') {
        printf("Unable to parse message length %s\n", argv[3]);
        return 1;
    }

    nbytes = strtol(argv[4], &endptr, 10);
    if (*endptr != '\0') {
        snprintf(message, sizeof(message), "Unable to parse number of bytes %s", argv[4]);
        perror(message);
        return 1;
    }

    buffer = malloc(nbytes);
    if (buffer == NULL) {
        printf("Failed to allocate buffer of %zu bytes\n", nbytes);
        return 1;
    }

    return 0;
}

void bench_thr_close(char *argv[]) {
    if (buffer != NULL) {
        free(buffer);
    }
    if ((test_gd >= 0) && (pirate_close(test_gd) < 0)) {
        snprintf(message, sizeof(message), "Unable to close test channel %s", argv[1]);
        perror(message);
    }
    if ((sync_gd >= 0) && (pirate_close(sync_gd) < 0)) {
        snprintf(message, sizeof(message), "Unable to close sync channel %s", argv[2]);
        perror(message);
    }
}
