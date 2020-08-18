#pragma once

#include <functional>

using FrameBuffer = const char *;
using ProcessFrameCallback = std::function<int(FrameBuffer, size_t)>;

class FrameProcessor
{
public:
    FrameProcessor();
    virtual ~FrameProcessor();

    const ProcessFrameCallback& getProcessFrameCallback();
    virtual int init() = 0;
    virtual void term() = 0;
protected:
    virtual int processFrame(FrameBuffer data, size_t length) = 0;
    unsigned mIndex;
private:
    int process(FrameBuffer data, size_t length);
    const ProcessFrameCallback mProcessFrameCallback;
};

