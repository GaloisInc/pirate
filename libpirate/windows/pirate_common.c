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

#include "libpirate.h"
#include "pirate_common.h"

int pirate_parse_key_value(char **key, char **val, char *ptr, char **saveptr) {
    *key = strtok_s(ptr, KV_DELIM, saveptr);
    if (*key == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    *val = strtok_s(NULL, KV_DELIM, saveptr);
    if (*val == NULL) {
        SetLastError(WSAEINVAL);
        return -1;
    }
    return 1;
}
