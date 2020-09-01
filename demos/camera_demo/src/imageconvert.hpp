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
