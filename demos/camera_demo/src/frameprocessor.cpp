#include "frameprocessor.hpp"

FrameProcessor::FrameProcessor() :
    mIndex(0),
    mProcessFrameCallback(std::bind(&FrameProcessor::process, this, 
                std::placeholders::_1, std::placeholders::_2))
{

}

FrameProcessor::~FrameProcessor()
{

}

const ProcessFrameCallback& FrameProcessor::getProcessFrameCallback()
{
    return mProcessFrameCallback;
}

int FrameProcessor::process(FrameBuffer data, size_t length)
{
    mIndex++;
    return processFrame(data, length);
}

