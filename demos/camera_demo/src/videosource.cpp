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

#include "videosource.hpp"

VideoSource::VideoSource(const Options& options,
    const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
        mImageConvert(ImageConvert(options.mImageWidth, options.mImageHeight)),
        mFrameProcessors(frameProcessors),
        mVerbose(options.mVerbose),
        mVideoOutputType(options.mVideoOutputType),
        mOutputWidth(options.mImageWidth),
        mOutputHeight(options.mImageHeight),
        mIndex(0),
        mSnapshotIndex(0),
        mSnapshotTime(0) {
}

VideoSource::~VideoSource() { }

int VideoSource::init() {
    mSnapshotTime = time(NULL);
    return 0;
}

void VideoSource::term() { }

int VideoSource::process(FrameBuffer data, size_t length) {
    int rv;
    time_t currentTime;

    mIndex++;
    for (size_t i = 0; i < mFrameProcessors.size(); i++) {
        auto current = mFrameProcessors[i];
        if (current->mVideoType == mVideoOutputType) {
            rv = current->processFrame(data, length);
            if (rv) {
                return rv;
            }
        } else {
            unsigned char* convertedBuffer = nullptr;
            size_t convertedLength = 0;
            rv = mImageConvert.convert(data, length, mVideoOutputType, current->mVideoType, mIndex);
            if (rv) {
                return rv;
            }
            convertedBuffer = mImageConvert.getBuffer(current->mVideoType, mIndex, &convertedLength);
            if (convertedBuffer != nullptr) {
                rv = current->processFrame(convertedBuffer, convertedLength);
                if (rv) {
                    return rv;
                }
            }
        }
    }

    if (mVerbose) {
        currentTime = time(NULL);
        if (currentTime != mSnapshotTime) {
            if ((currentTime % 10) == 0) {
                std::cout << (mIndex - mSnapshotIndex) << " frames per second" << std::endl;
            }
            mSnapshotIndex = mIndex;
            mSnapshotTime = currentTime;
        }
    }

    return 0;
}
