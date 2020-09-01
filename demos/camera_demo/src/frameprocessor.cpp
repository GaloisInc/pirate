#include "frameprocessor.hpp"

FrameProcessor::FrameProcessor(VideoType videoType, unsigned width, unsigned height) :
    mVideoType(videoType),
    mImageWidth(width),
    mImageHeight(height),
    mIndex(0)
{

}

FrameProcessor::~FrameProcessor()
{

}

int FrameProcessor::process(FrameBuffer data, size_t length)
{
    mIndex++;
    return processFrame(data, length);
}

