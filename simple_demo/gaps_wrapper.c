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
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define USAGE(name) fprintf(stderr, "%s [high|low] <args ...>\n", name)

/* Symbols from the linked binary blobs */
extern uint8_t _binary_high_start;
extern uint8_t _binary_high_end;
extern uint8_t _binary_low_start;
extern uint8_t _binary_low_end;

static int run_bin_blob(const char* name, uint8_t* start, uint8_t* end, 
                        char* argv[], char* envp[]) {
    long len = end - start;

    int fd = memfd_create(name, MFD_CLOEXEC);
    if (fd == -1) {
        perror("memfd_create: failed to create mapped file\n");
        return -1;
    }

    if (write(fd, start, len) != len) {
        perror("write: failed to write shared memory file\n");
        return -1;
    }

    if (fexecve(fd, argv, envp) == -1) {
        perror("fexecve: failed to start the program");
        return -1;
    }

    /* Should never get here */
    return -1;
}

int main(int argc, char* argv[], char* envp[]) {
    if (argc < 2) {
        USAGE(argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "high") == 0) {
        return run_bin_blob("high_elf", &_binary_high_start, &_binary_high_end, 
                            &argv[1], envp);
    } else if (strcmp(argv[1], "low") == 0) {
        return run_bin_blob("low_elf", &_binary_low_start, &_binary_low_end,
                            &argv[1], envp);
    } else {
        USAGE(argv[0]);
        return -1;
    }

    /* Should never get here */
    return -1;
}
