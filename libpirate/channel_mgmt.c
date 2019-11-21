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
//
// An example of using C constructors and destructors to initialize
// channels. This code is not compiled into the libpirate library.
//
// To be determined whether this functionality should live in the
// pirate primitives, or this functionality should be inserted by
// the GAPS compiler toolchain, or initialization and termination
// should be explicit operations (not using the constructor).

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "primitives.h"

#define PIRATE_INVALID_CHANNEL  -1

#define PIRATE_CTOR_PRIO        101
#define PIRATE_DTOR_PRIO        PIRATE_CTOR_PRIO

typedef struct {
    int num;        // Channel number
    int gd;         // Channel descriptor
    int flags;      // O_RDONLY or O_WRONLY
    char* desc;     // Channel description string
} pirate_channel_desc_t;

#define PIRATE_CHANNEL_CONFIG(num, flags, desc)  {num, -1, flags, desc}
#define PIRATE_END_CHANNEL_CONFIG   {PIRATE_INVALID_CHANNEL, -1, 0, NULL}

#define PIRATE_NO_CHANNELS                                               \
pirate_channel_desc_t pirate_channels[] = {                              \
    PIRATE_END_CHANNEL_CONFIG                                            \
}

extern pirate_channel_desc_t pirate_channels[];

void __attribute__ ((destructor(PIRATE_DTOR_PRIO))) pirate_channel_term() {
    pirate_channel_desc_t* ch;
    for (ch = pirate_channels; ch->num != PIRATE_INVALID_CHANNEL; ch++) {
        if (ch->gd < 0) {
            continue;
        }

        pirate_close(ch->num, ch->flags);
        ch->gd = -1;
    }
}

void __attribute__ ((constructor(PIRATE_CTOR_PRIO)))
pirate_channel_init(int argc, char* argv[], char* envp[]) {
    (void) argc, (void)argv, (void) envp;
    pirate_channel_desc_t* ch;

    /*
     * The destructor will only be called when main returns, ensure that other
     * cases are covered
     */
    if (atexit(pirate_channel_term) != 0) {
        exit(-1);
    }

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = pirate_channel_term;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        exit(-1);
    }

    /* Open configured channels */
    for (ch = pirate_channels; ch->num != PIRATE_INVALID_CHANNEL; ch++)
    {
        ch->gd = pirate_open(ch->num, ch->flags);
        if (ch->gd == -1) {
            exit(-1);
        }
    }
}