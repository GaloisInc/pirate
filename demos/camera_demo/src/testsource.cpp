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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "options.hpp"
#include "testsource.hpp"

TestSource::TestSource(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
    VideoSource(options, frameProcessors),
    mBuffer(nullptr),
    mPollThread(nullptr),
    mPoll(false)
{
}

TestSource::~TestSource()
{
    term();
}

int TestSource::init()
{
    int rv;

    rv = VideoSource::init();
    if (rv) {
        return rv;
    }

    mBuffer = (uint8_t*) calloc(mOutputWidth * mOutputHeight * 4, sizeof(uint8_t));
    memset(mBuffer, 127, mOutputWidth * mOutputHeight * 4);

    // Start the capture thread
    mPoll = true;
    mPollThread = new std::thread(&TestSource::pollThread, this);

    return 0;
}

void TestSource::term()
{
    if (mPoll)
    {
        mPoll = false;
        if (mPollThread != nullptr)
        {
            mPollThread->join();
            delete mPollThread;
            mPollThread = nullptr;
        }
    }
    if (mBuffer != nullptr)
    {
        free(mBuffer);
    }
}

void TestSource::perturb()
{
    for (unsigned int col = 0; col < mOutputHeight; col++) {
        for (unsigned int row = 0; row < mOutputWidth; row++) {
            uint8_t *offset = mBuffer + ((col * mOutputWidth) + row) * 4;
            offset[0] += 1;
            offset[1] += 2;
            offset[2] += 3;
        }
    }
}

void TestSource::pollThread()
{
    int rv;
    struct timespec req;

    req.tv_sec = 0;
    req.tv_nsec = 1e9 / 60;
    while (mPoll)
    {
        perturb();
        // Process the frame
        rv = process(mBuffer, mOutputWidth * mOutputHeight * 4, VideoData);
        if (rv) {
            std::cout << "frame processor error " << rv << std::endl;
        }
        nanosleep(&req, NULL);
    }
}
