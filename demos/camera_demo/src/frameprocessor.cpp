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

#include "frameprocessor.hpp"

FrameProcessor::FrameProcessor(VideoType videoType, unsigned width, unsigned height) :
    mVideoType(videoType),
    mImageWidth(width),
    mImageHeight(height),
    mVideoIndex(0)
{

}

FrameProcessor::~FrameProcessor()
{

}

int FrameProcessor::processFrame(FrameBuffer data, size_t length, DataStreamType dataStream)
{
    if (dataStream == VideoData) {
        mVideoIndex++;
    }
    return process(data, length, dataStream);
}

