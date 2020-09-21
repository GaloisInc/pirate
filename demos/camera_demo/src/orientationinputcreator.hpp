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

#pragma once

#include <iostream>
#include <stdexcept>
#include "keyboardorientationinput.hpp"

#if FREESPACE_PRESENT
#include "freespaceorientationinput.hpp"
#endif

#include "options.hpp"

class OrientationInputCreator {
public:
    static OrientationInput * get(InputType inputType, const Options& options,
            CameraOrientationCallbacks angPosCallbacks)
    {
        switch (inputType)
        {
#if FREESPACE_PRESENT
            case Freespace:
                if (options.mVerbose) {
                    std::cout << "Using Freespace device" << std::endl;
                }
                return new FreespaceOrientationInput(angPosCallbacks);
#endif
            case Keyboard:
                if (options.mVerbose) {
                    std::cout << "Using keyboard" << std::endl;
                }
                return new KeyboardOrientationInput(angPosCallbacks);
            default:
                throw std::runtime_error("Unsupported orientation input");
                return nullptr;
        }
    }
};
