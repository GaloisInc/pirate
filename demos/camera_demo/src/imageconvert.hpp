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

class ImageConvert
{
public:
    ImageConvert(unsigned width, unsigned height);
    ~ImageConvert();

    int convert(FrameBuffer src, size_t srcLength, VideoType srcType, unsigned char* dst, VideoType dstType) const;
    unsigned char* getBuffer(VideoType videoType) const;

    static size_t expectedBytes(unsigned width, unsigned height, VideoType videoType);
private:

    const unsigned mImageWidth;
    const unsigned mImageHeight;

    unsigned char* mTempJpegBuffer;
    unsigned char* mTempJpegBufferRow;
    unsigned char* mRGBXBuffer;

    int convertJpegToRGBX(FrameBuffer src, size_t srcLength, unsigned char* dst) const;
    int convertYUYVToRGBX(FrameBuffer src, unsigned char* dst) const;
};
