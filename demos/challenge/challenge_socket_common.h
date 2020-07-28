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

#ifndef _CHALLENGE_SOCKET_COMMON_H
#define _CHALLENGE_SOCKET_COMMON_H

#include <unistd.h>

int reader_open(int port);
int writer_open(int port);
void test_rv(ssize_t rv, ssize_t nbytes, const char *msg);

#endif // _CHALLENGE_SOCKET_COMMON_H
