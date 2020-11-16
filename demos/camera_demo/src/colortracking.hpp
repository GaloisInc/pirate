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

#include <thread>

#include "orientationinput.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"

class ColorTracking : public OrientationInput, public FrameProcessor
{
public:
    ColorTracking(const Options& options,
        CameraOrientationCallbacks angPosCallbacks);
    virtual ~ColorTracking();

    virtual int init() override;
    virtual void term() override;

private:
    const bool           mVerbose;
    const float          mPanAxisMin;
    const float          mPanAxisMax;
    const float          mTiltAxisMin;
    const float          mTiltAxisMax;
    const float          mAngIncrement;
    const bool           mImageSlidingWindow;
    const unsigned char  mImageTrackingRGB[3];
    const unsigned       mImageTrackingThreshold;

    void computeTracking(int* x_pos, int *y_pos, FrameBuffer data);

protected:
    virtual int process(FrameBuffer data, size_t length, DataStreamType dataStream) override;
};
