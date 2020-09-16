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
    const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors,
    const ImageConvert& imageConvert) :
        mFrameProcessors(frameProcessors),
        mImageConvert(imageConvert),
        mVerbose(options.mVerbose),
        mVideoOutputType(VideoSource::videoInputToOutput(options.mVideoInputType)),
        mOutputWidth(options.mImageWidth),
        mOutputHeight(options.mImageHeight),
        mIndex(0),
        mSnapshotIndex(0),
        mSnapshotTime(0) {
}

VideoSource::~VideoSource() { }


VideoType VideoSource::videoInputToOutput(VideoType videoInput) {
    switch (videoInput) {
        case JPEG:
        case YUYV:
        case RGBX:
        case H264:
            return videoInput;
        case STREAM:
            return YUYV;
    }
}

int VideoSource::process(FrameBuffer data, size_t length) {
    int rv;
    time_t currentTime;

    mIndex++;
    for (size_t i = 0; i < mFrameProcessors.size(); i++) {
        auto current = mFrameProcessors[i];
        if (current->mVideoOutputType == mVideoOutputType) {
            rv = current->processFrame(data, length);
            if (rv) {
                return rv;
            }
        } else {
            unsigned char* convertedBuffer = nullptr;
            size_t convertedLength = 0;
            // check whether the image has already been converted
            // in a previous frame processor
            for (size_t j = 0; j < i; j++) {
                auto prev = mFrameProcessors[j];
                convertedBuffer = prev->getFrame(mIndex, current->mVideoOutputType, &convertedLength);
                if (convertedBuffer != nullptr) {
                    break;
                }
            }
            // otherwise attempt to convert the image
            if ((convertedBuffer == nullptr) &&
                (convertedBuffer = mImageConvert.getBuffer(current->mVideoOutputType, &convertedLength)) != nullptr) {
                mImageConvert.convert(data,
                        length,
                        mVideoOutputType,
                        convertedBuffer,
                        current->mVideoOutputType);
            }
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
