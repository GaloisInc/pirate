#pragma once

#include <stdint.h>
#include <string>
#include <thread>
#include <linux/videodev2.h>
#include "frameprocessor.hpp"

class VideoSensor
{
public:
    VideoSensor(const ProcessFrameCallback& processFrameCallback,
            std::string devicePath = "/dev/video0",
            bool hFlip = true, bool vFlip = true,
            unsigned imgWidth = DEFAULT_IMAGE_WIDTH,
            unsigned imgHeight = DEFAULT_IMAGE_HEIGHT);
    virtual ~VideoSensor();

    virtual int init();
    virtual void term();

    virtual int captureEnable(bool enable);
    static constexpr unsigned DEFAULT_IMAGE_WIDTH = 3280;
    static constexpr unsigned DEFAULT_IMAGE_HEIGHT = 2464;
private:
    const ProcessFrameCallback& mProcessFrameCallback;
    
    const std::string mDevicePath;
    const bool mFlipHorizontal;
    const bool mFlipVertical;
    unsigned mImageWidth;
    unsigned mImageHeight;
    unsigned mFrameSize;

    static constexpr unsigned BUFFER_COUNT = 4;
    
    int mFd;
    struct v4l2_capability mCapability;
    struct v4l2_format mFormat;
    struct v4l2_requestbuffers mRequestBuffers;
    
    struct VideoBuffer
    {
        VideoBuffer() : mStart(nullptr), mLength(0) {}

        char * mStart;
        size_t mLength;
    };

    VideoBuffer mBuffers[BUFFER_COUNT];

    static int ioctlWait(int fd, unsigned long req, void *arg);

    int openVideoDevice();
    int closeVideoDevice();
    int initVideoDevice();
    int uninitVideoDevice();
    int initCaptureBuffers();
    int releaseCaptureBuffers();

    std::thread *mPollThread;
    bool mPoll;
    void pollThread();
};

