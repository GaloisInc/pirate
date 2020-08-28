#pragma once

#include <stdint.h>
#include <string>
#include <thread>
#include <linux/videodev2.h>
#include "frameprocessor.hpp"
#include "options.hpp"

class VideoSensor
{
public:
    VideoSensor(const Options& options, const ProcessFrameCallback& processFrameCallback);
    virtual ~VideoSensor();

    virtual int init();
    virtual void term();

    virtual int captureEnable();
    virtual int captureDisable();
private:
    const ProcessFrameCallback& mProcessFrameCallback;
    
    const std::string mDevicePath;
    const VideoType mVideoType;
    const bool mFlipHorizontal;
    const bool mFlipVertical;
    unsigned mImageWidth;
    unsigned mImageHeight;
    const unsigned mFrameRateNumerator;
    const unsigned mFrameRateDenominator;

    static constexpr unsigned BUFFER_COUNT = 4;
    
    int mFd;
    struct v4l2_capability mCapability;
    struct v4l2_format mFormat;
    struct v4l2_requestbuffers mRequestBuffers;
    
    struct VideoBuffer
    {
        VideoBuffer() : mStart(nullptr), mLength(0) {}

        unsigned char * mStart;
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

