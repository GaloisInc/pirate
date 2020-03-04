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

#include <stdio.h>

#pragma pirate capability declare(red)
#pragma pirate enclave declare(alpha)
#pragma pirate enclave declare(beta)
#pragma pirate enclave declare(gamma)
#pragma pirate enclave capability(alpha, red)

void onalpha(void)
    __attribute__((pirate_enclave_only("alpha")))
    __attribute__((pirate_capability("red")))
{
    printf("running on alpha\n");
}

void alphamain(void)
    __attribute__((pirate_enclave_main("alpha")))
{
    onalpha();
    printf("alpha started\n");
}

void betamain(void)
    __attribute__((pirate_enclave_main("beta")))
{
    printf("beta started\n");
}

void gammamain(void)
    __attribute__((pirate_enclave_main("gamma")))
{
    printf("gamma started\n");
}
