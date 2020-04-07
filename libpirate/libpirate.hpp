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

#ifndef __PIRATE_PRIMITIVES_CXX_H
#define __PIRATE_PRIMITIVES_CXX_H

#include <functional>

namespace pirate {
    namespace internal {
        int cooperative_register(int gd, void* listener, size_t len);

        template <typename T>
        union listener_union_hack {
            std::function<void(const T& val)> *listener;
            void *ptr;
        };
    }
}

// Register a listener on the gaps channel.
//
// The gaps channel must be opened with access mode
// O_RDONLY. The gaps channel must be a yield channel.
// The gaps channel must not be a control channel.
// The listeners registered on the same gaps channel
// must accept a T with equal size (sizeof(T)).

template <typename T>
int pirate_register_listener(int gd, std::function<void(const T& val)> listener) {
    pirate::internal::listener_union_hack<T> lu;
    lu.listener = &listener;
    return pirate::internal::cooperative_register(gd, lu.ptr, sizeof(T));
}

#endif
