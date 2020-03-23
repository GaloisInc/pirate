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

#include <iostream>
#include <cerrno>

#include <string.h>

#include "libpirate.h"

#define BUF_SIZE 1024

static char buffer[BUF_SIZE];

int main(int argc, char* argv[]) {
    int gd, rv, len;
    pirate_channel_param_t param;

    pirate_init_channel_param(PIPE, &param);

    gd = pirate_open_param(&param, O_WRONLY);
    if (gd < 0) {
        perror("pirate_open");
        return 1;
    }

    while(std::cin.getline(buffer, BUF_SIZE)) {
        // send the terminating null byte
        len = strlen(buffer) + 1;
        rv = pirate_write(gd, &len, sizeof(len));
        if (rv != sizeof(len)) {
            std::cerr << "write error" << "\n";
            return 1;
        }
        rv = pirate_write(gd, buffer, len);
        if (rv != len) {
            std::cerr << "write error" << "\n";
            return 1;
        }
    }

    if (errno) {
        perror("getline");
        return 1;
    }

    len = 0;
    rv = pirate_write(gd, &len, sizeof(len));
    if (rv != sizeof(len)) {
        std::cerr << "write error" << "\n";
        return 1;
    }

    return 0;
}
