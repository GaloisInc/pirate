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

// Angular position is read from the keyboard left/right keys

#include <thread>
#include <termios.h>

#include "options.hpp"
#include "cameracontrolinput.hpp"

class KeyboardCameraControlInput : public CameraControlInput
{
public:
    KeyboardCameraControlInput(const Options& options,
                             CameraControlCallbacks cameraControlCallbacks);
    virtual ~KeyboardCameraControlInput();

    virtual int init();
    virtual void term();

private:
    const float mAngIncrement;

    enum eKeyCode {
        UP       = 0x41,
        DOWN       = 0x42,
        RIGHT      = 0x43,
        LEFT       = 0x44,
        ZOOM_INC   = 0x2B,
        ZOOM_DEC   = 0x2D,
        ZOOM_RESET = 0x3D
    };

    struct termios mTermiosBackup;
    bool mTermiosInit;

    std::thread *mPollThread;
    bool mPoll;
    void pollThread();
};
