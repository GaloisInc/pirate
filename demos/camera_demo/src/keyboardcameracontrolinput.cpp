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

#include <cstdio>
#include <unistd.h>
#include <sys/select.h>
#include "keyboardcameracontrolinput.hpp"

// Debug
#include <iomanip>
#include <iostream>

KeyboardCameraControlInput::KeyboardCameraControlInput(
        const Options& options, CameraControlCallbacks cameraControlCallbacks) :
    CameraControlInput(cameraControlCallbacks),
    mAngIncrement(options.mAngularPositionIncrement),
    mTermiosInit(false),
    mPollThread(nullptr),
    mPoll(false)
{

}

KeyboardCameraControlInput::~KeyboardCameraControlInput()
{
    term();
}

int KeyboardCameraControlInput::init()
{
    // Setup stdin to be read one key at a time
    int rv = tcgetattr(0, &mTermiosBackup);
    if (rv)
    {
        std::perror("tcgetattr failed");
        return -1;
    }

    struct termios raw = mTermiosBackup;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;

    rv = tcsetattr(0, TCSANOW, &raw);
    if (rv)
    {
        std::perror("tcsetattr failed");
        return -1;
    }
    mTermiosInit = true;

    // Start the reading thread
    mPoll = true;
    mPollThread = new std::thread(&KeyboardCameraControlInput::pollThread, this);

    return 0;
}

void KeyboardCameraControlInput::term()
{
    // Stop the polling thread
    if (mPollThread != nullptr)
    {
        mPoll = false;
        mPollThread->join();
        delete mPollThread;
        mPollThread = nullptr;
    }

    if (mTermiosInit)
    {
        // Restore stdio defaults
        int rv = tcsetattr(0, TCSANOW, &mTermiosBackup);
        if (rv)
        {
            std::perror("tcsetattr failed");
        }
    }
}

void KeyboardCameraControlInput::pollThread()
{
    while (mPoll)
    {
        PanTilt panTiltUpdate = PanTilt();
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet); // stdin

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        int rv = select(1, &fdSet, NULL, NULL, &timeout);

        if (rv == -1)
        {
            std::perror("select failed");
            mPoll = false;
            return;
        }
        else if (rv == 0)
        {
            continue;       // Timeout
        }

        char c;
        rv = read(0, &c, 1);
        if (rv != 1)
        {
            std::perror("read failed");
            mPoll = false;
            return;
        }

        switch (c)
        {
            case UP:
                panTiltUpdate.tilt = mAngIncrement;
                mCallbacks.mPosUpdate(panTiltUpdate);
                break;
            case DOWN:
                panTiltUpdate.tilt = -mAngIncrement;
                mCallbacks.mPosUpdate(panTiltUpdate);
                break;
            case LEFT:
                panTiltUpdate.pan = -mAngIncrement;
                mCallbacks.mPosUpdate(panTiltUpdate);
                break;
            case RIGHT:
                panTiltUpdate.pan = mAngIncrement;
                mCallbacks.mPosUpdate(panTiltUpdate);
                break;
            case ZOOM_INC:
                mCallbacks.mZoomUpdate(Increment);
                break;
            case ZOOM_DEC:
                mCallbacks.mZoomUpdate(Decrement);
                break;
            case ZOOM_RESET:
                mCallbacks.mZoomUpdate(Reset);
                break;
            default:
                continue;
        }
    }
}
