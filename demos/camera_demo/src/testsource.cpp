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
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "options.hpp"
#include "testsource.hpp"

#include "orion-sdk/Constants.hpp"
#include "orion-sdk/KlvParser.hpp"
#include "orion-sdk/scaledencode.hpp"

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
    int index, rv;
    double scale_lat, scale_lon;
    struct timespec req;
    uint8_t klv_data[40];
    uint64_t counter = 0;

    req.tv_sec = 0;
    req.tv_nsec = 1e9 / 60;
    memset(klv_data, 0, sizeof(klv_data));
    scale_lat = pow(2, 64) / (radians(90) - radians(-90));
    scale_lon = pow(2, 64) / (radians(180) - radians(-180));

    klv_data[16] = 20;
    klv_data[17] = KLV_UAS_SENSOR_LAT;
    klv_data[18] = 8;
    klv_data[27] = KLV_UAS_SENSOR_LON;
    klv_data[28] = 8;
    index = 19;
    float64ScaledTo8SignedBeBytes(radians(39.833333), klv_data, &index, scale_lat);
    index = 29;
    float64ScaledTo8SignedBeBytes(radians(-98.583333), klv_data, &index, scale_lon);

    while (mPoll)
    {
        counter++;
        perturb();

        // Broadcast the metadata every frame for 5 seconds.
        // Then stop broadcasting the metadata for 5 seconds.
        // Repeat.
        if ((counter % 600) < 300) {
            rv = process(klv_data, 40, MetaData);
            if (rv) {
                std::cout << "metadata frame processor error " << rv << std::endl;
            }
        } else if ((counter % 600) == 300) {
            double lat_degrees = drand48() * (49.3457868 - 24.743319) + 24.743319;
            double lon_degrees = drand48() * (-66.9513812 - (-124.7844079)) + (-124.7844079);
            index = 19;
            float64ScaledTo8SignedBeBytes(radians(lat_degrees), klv_data, &index, scale_lat);
            index = 29;
            float64ScaledTo8SignedBeBytes(radians(lon_degrees), klv_data, &index, scale_lon);
        }
        rv = process(mBuffer, mOutputWidth * mOutputHeight * 4, VideoData);
        if (rv) {
            std::cout << "video frame processor error " << rv << std::endl;
        }

        nanosleep(&req, NULL);
    }
}
