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
    int process(FrameBuffer data, size_t length);
    virtual unsigned char* getFrame(unsigned index, VideoType videoType) = 0;

    const VideoType mVideoType;
    const unsigned  mImageWidth;
    const unsigned  mImageHeight;

protected:
    unsigned     mIndex;
    virtual int processFrame(FrameBuffer data, size_t length) = 0;

};
