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

#include <functional>

#include "options.hpp"

class FrameProcessor
{
public:
    FrameProcessor(VideoType videoType, unsigned width, unsigned height);
    virtual ~FrameProcessor();

    virtual int init() = 0;
    virtual void term() = 0;
    int processFrame(FrameBuffer data, size_t length, DataStreamType dataStream);

    const VideoType mVideoType;
    const unsigned  mImageWidth;
    const unsigned  mImageHeight;

protected:
    unsigned    mVideoIndex;
    virtual int process(FrameBuffer data, size_t length, DataStreamType dataStream) = 0;

};
