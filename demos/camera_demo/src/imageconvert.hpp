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

    int convert(FrameBuffer src, size_t srcLength, VideoType srcType, VideoType dstType, unsigned index);
    unsigned char* getBuffer(VideoType dstType, unsigned index, size_t* length) const;
private:

    const unsigned mImageWidth;
    const unsigned mImageHeight;

    unsigned char* mYUYVBuffer;
    unsigned char* mBGRXBuffer;

    unsigned mYUYVIndex;
    unsigned mBGRXIndex;

    unsigned char* mRGBBuffer;
    unsigned char* mRGBBufferRow;

    int jpegToRGB(FrameBuffer src, size_t length);
    int rgbToBGRX();
    int rgbToYUYV();

    int jpegToBGRX(FrameBuffer src, size_t length, unsigned index);
    int jpegToYUYV(FrameBuffer src, size_t length, unsigned index);
    int yuyvToBGRX(FrameBuffer src, size_t length, unsigned index);
};
