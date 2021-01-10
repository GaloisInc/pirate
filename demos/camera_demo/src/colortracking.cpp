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

#include <iostream>

#include "colortracking.hpp"

ColorTracking::ColorTracking(
        const Options& options,
        CameraControlCallbacks cameraControlCallbacks) :
    CameraControlInput(cameraControlCallbacks),
    FrameProcessor(VIDEO_BGRX, options.mImageWidth, options.mImageHeight),
    mVerbose(options.mVerbose),
    mPanAxisMin(options.mPanAxisMin),
    mPanAxisMax(options.mPanAxisMax),
    mTiltAxisMin(options.mTiltAxisMin),
    mTiltAxisMax(options.mTiltAxisMax),
    mAngIncrement(options.mAngularPositionIncrement),
    mImageSlidingWindow(options.mImageSlidingWindow),
    mImageTrackingRGB{options.mImageTrackingRGB[0], options.mImageTrackingRGB[1], options.mImageTrackingRGB[2]},
    mImageTrackingThreshold(options.mImageTrackingThreshold)
{
}

ColorTracking::~ColorTracking()
{
    term();
}

int ColorTracking::init()
{
    return 0;
}

void ColorTracking::term()
{

}

void ColorTracking::computeTracking(int* x_pos, int *y_pos, FrameBuffer data) {
    unsigned int x, y, k;
    unsigned int count = 0;
    unsigned int x_sum = 0, y_sum = 0;
    *x_pos = -1;
    *y_pos = -1;

    for(k = y = 0; y < mImageHeight; y++) {
	    for(x = 0; x < mImageWidth; x++, k += 4) {
            // reference https://www.compuphase.com/cmetric.htm
            int64_t rmean = (((int64_t) data[k+2]) + ((int64_t) mImageTrackingRGB[0])) / 2;
            int64_t r = ((int64_t) data[k+2]) - ((int64_t) mImageTrackingRGB[0]);
            int64_t g = ((int64_t) data[k+1]) - ((int64_t) mImageTrackingRGB[1]);
            int64_t b = ((int64_t) data[k+0]) - ((int64_t) mImageTrackingRGB[2]);
            uint64_t delta = (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8);
            if (delta < mImageTrackingThreshold) {
                // TODO: weighted average based on color similarity?
                count++;
                x_sum += x;
                y_sum += y;
            }
        }
    }
    if (mVerbose) {
        std::cout << count << " pixels match color threshold" << std::endl;
    }
    if (count > 64) {
        *x_pos = x_sum / count;
        *y_pos = y_sum / count;
    }
}

int ColorTracking::process(FrameBuffer data, size_t length, DataStreamType dataStream) {
    int x_center, y_center;
    int x_delta, y_delta;
    int x_position, y_position;
    int x_tolerance = mImageWidth / 10; // 10% tolerance
    int y_tolerance = mImageHeight / 10; // 10% tolerance
    PanTilt update = PanTilt(0.0, 0.0);

    if (dataStream != VideoData) {
        return 0;
    }

    if (length != (mImageWidth * mImageHeight * 4)) {
        return -1;
    }

    computeTracking(&x_position, &y_position, data);
    if ((x_position < 0) || (y_position < 0)) {
        // object not found
        return 0;
    }

    if (mImageSlidingWindow) {
        PanTilt angularPosition = mCallbacks.mPosGet();
        float fractionX = (angularPosition.pan - mPanAxisMin) / (mPanAxisMax - mPanAxisMin);
        float fractionY = (-angularPosition.tilt - mTiltAxisMin) / (mTiltAxisMax - mTiltAxisMin);
        x_center = mImageWidth * fractionX;
        y_center = mImageHeight * fractionY;
    } else {
        x_center = mImageWidth / 2;
        y_center = mImageHeight / 2;
    }

    x_delta = x_position - x_center;
    y_delta = y_position - y_center;

    if (x_delta > x_tolerance) {
        update.pan = mAngIncrement;
    } else if (x_delta < -x_tolerance) {
        update.pan = -mAngIncrement;
    }

    if (y_delta > y_tolerance) {
        update.tilt = -mAngIncrement;
    } else if (y_delta < -y_tolerance) {
        update.tilt = mAngIncrement;
    }

    if ((update.pan != 0.0) || (update.tilt != 0.0)) {
        mCallbacks.mPosUpdate(update);
    }

    return 0;
}
