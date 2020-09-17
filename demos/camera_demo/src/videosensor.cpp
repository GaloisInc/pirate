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

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <cerrno>
#include <cstring>
#include <iostream>

#include "options.hpp"
#include "videosensor.hpp"

VideoSensor::VideoSensor(const Options& options,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors) :
    VideoSource(options, frameProcessors),
    mDevicePath(options.mVideoDevice),
    mFlipHorizontal(options.mImageHorizontalFlip),
    mFlipVertical(options.mImageVerticalFlip),
    mFrameRateNumerator(options.mFrameRateNumerator),
    mFrameRateDenominator(options.mFrameRateDenominator),
    mFd(-1),
    mPollThread(nullptr),
    mPoll(false)
{
    std::memset(&mCapability, 0, sizeof(mCapability));
    std::memset(&mFormat, 0, sizeof(mFormat));
    std::memset(&mRequestBuffers, 0, sizeof(mRequestBuffers));
}

VideoSensor::~VideoSensor()
{
    term();
}

int VideoSensor::init()
{
    int rv;

    rv = VideoSource::init();
    if (rv) {
        return rv;
    }

    // Open the video device
    rv = openVideoDevice();
    if (rv != 0)
    {
        return -1;
    }

    // Configure the video device
    rv = initVideoDevice();
    if (rv != 0)
    {
        return -1;
    }

    rv = captureEnable();
    if (rv != 0)
    {
        return -1;
    }

    // Start the capture thread
    mPoll = true;
    mPollThread = new std::thread(&VideoSensor::pollThread, this);

    return 0;
}

void VideoSensor::term()
{
    if (mPoll)
    {
        mPoll = false;
        if (mPollThread != nullptr)
        {
            mPollThread->join();
            delete mPollThread;
            mPollThread = nullptr;
        }
    }
    captureDisable();
    uninitVideoDevice();
    closeVideoDevice();
}

int VideoSensor::captureEnable()
{
    int rv;

    struct v4l2_buffer buf;
        
    for (unsigned i = 0; i < BUFFER_COUNT; i++)
    {
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        rv = ioctlWait(mFd, VIDIOC_QBUF, &buf);
        if (rv != 0)
        {
            std::perror("Failed to queue the buffer");
            return -1;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int cmd = VIDIOC_STREAMON;

    rv = ioctlWait(mFd, cmd, &type);
    if (rv != 0)
    {
        std::perror("Failed to set video ON mode");
        return -1;
    }

    return 0;
}

int VideoSensor::captureDisable()
{
    int rv;

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int cmd = VIDIOC_STREAMOFF;

    rv = ioctlWait(mFd, cmd, &type);
    if (rv != 0)
    {
        std::perror("Failed to set video OFF mode");
        return -1;
    }

    return 0;
}

int VideoSensor::ioctlWait(int fd, unsigned long req, void * arg)
{
    int retry = 0;
    int ret = -1;

    do
    {
        retry = 0;
        int err = errno;
        ret = ioctl(fd, req, arg);
        if ((ret == -1) && (errno == EINTR))
        {
            retry = 1;
            errno = err;
        }
    } while (retry);

    return ret;
}

int VideoSensor::openVideoDevice()
{
    int rv;
    struct stat st;

    rv = stat(mDevicePath.c_str(), &st);
    if (rv == -1)
    {
        std::perror("Failed to stat the video device");
        return -1;
    }

    if (S_ISCHR(st.st_mode) == 0)
    {
        std::perror("Not a video device");
        return -1;
    }

    mFd = open(mDevicePath.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (mFd < 0)
    {
        std::perror("Failed to open the video device");
        return -1;
    }

    std::cout << "Video device opened" << std::endl;
    return 0;
}

int VideoSensor::closeVideoDevice()
{
    if (mFd > 0)
    {
        close(mFd);
        mFd = -1;
    }

    std::cout << "Video device closed" << std::endl;
    return 0;
}

int VideoSensor::initVideoDevice()
{
    int rv;

    // Query video device capabilities
    std::memset(&mCapability, 0, sizeof(mCapability));
    rv = ioctlWait(mFd, VIDIOC_QUERYCAP, &mCapability);
    if (rv != 0)
    {
        std::perror("Failed to query video device capabilites");
        return -1;
    }

    if ((mCapability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
    {
        errno = ENOSYS;
        std::perror("Device does not support capture");
        return -1;
    }

    if ((mCapability.capabilities & V4L2_CAP_STREAMING) == 0)
    {
        errno  = ENOSYS;
        std::perror("Device does not support streaming");
        return -1;
    }

    // Default cropping
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;

    std::memset(&cropcap, 0, sizeof(cropcap));
    std::memset(&crop, 0, sizeof(crop));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctlWait(mFd, VIDIOC_CROPCAP, &cropcap) == 0)
    {
        // Video cropping is supported
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        // Errors are not critical
        rv = ioctlWait(mFd, VIDIOC_S_CROP, &crop);
        if ((rv != 0) && (errno != ENOTTY))
        {
            std::perror("Unable to set cropping");
        }
    }

    // Horizontal flip
    if (mFlipHorizontal)
    {
        struct v4l2_control ctrl;
        std::memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = V4L2_CID_HFLIP;
        ctrl.value = 1;

        rv = ioctlWait(mFd, VIDIOC_S_CTRL, &ctrl);
        if (rv != 0)
        {
            std::perror("V4L2: failed to set camera horizontal flip mode");
            return -1;
        }
    }

    // Vertical flip
    if (mFlipVertical)
    {
        struct v4l2_control ctrl;
        std::memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = V4L2_CID_VFLIP;
        ctrl.value = 1;

        rv = ioctlWait(mFd, VIDIOC_S_CTRL, &ctrl);
        if (rv != 0)
        {
            std::perror("V4L2: failed to set camera vertical flip mode");
            return -1;
        }
    }

    // Configure image format
    std::memset(&mFormat, 0, sizeof(mFormat));

    mFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mFormat.fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
    rv = ioctlWait(mFd, VIDIOC_G_FMT, &mFormat);
    if (rv)
    {
        std::perror("Failed to get the format configuration");
        return -1;
    }

    mFormat.fmt.pix.width = mOutputWidth;
    mFormat.fmt.pix.height = mOutputHeight;
    switch (mVideoOutputType) {
        case JPEG:
            mFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
            break;
        case YUYV:
            mFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
            break;
        case H264:
            mFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
            break;
        default:
            std::cout << "Unknown video type " << mVideoOutputType << std::endl;
            return -1;
    }
    rv = ioctlWait(mFd, VIDIOC_S_FMT, &mFormat);
    if (rv < 0)
    {
        std::perror("Failed to set the format configuration");
        return -1;
    }
    
    if ((mFormat.fmt.pix.width != mOutputWidth) ||
        (mFormat.fmt.pix.height != mOutputHeight))
    {
        errno = EINVAL;
        std::perror("Image resolution is not supported");
        return -1;
    }

    // Get frame rate

    // Note: the supported frame rate of the camera
    // can change based on the image format

    struct v4l2_streamparm streamparm;
    std::memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rv = ioctlWait(mFd, VIDIOC_G_PARM, &streamparm);
    if (rv != 0)
    {
        std::perror("Failed to get stream parameters");
        return -1;
    }

    // Compare in frames per second (not seconds per frame)
    double expFrameRate = ((double) mFrameRateDenominator) / ((double) mFrameRateNumerator);
    double obsFrameRate = ((double) streamparm.parm.capture.timeperframe.denominator) /
        ((double) streamparm.parm.capture.timeperframe.numerator);
    double delta = fabs(expFrameRate - obsFrameRate);

    // Set frame rate
    if (delta >= 1.0)
    {
        if (mCapability.capabilities & V4L2_CAP_TIMEPERFRAME)
        {
            streamparm.parm.capture.timeperframe.numerator = mFrameRateNumerator;
            streamparm.parm.capture.timeperframe.denominator = mFrameRateDenominator;
            rv = ioctlWait(mFd, VIDIOC_S_PARM, &streamparm);
            if (rv != 0)
            {
                std::perror("Failed to set stream parameters");
                return -1;
            }
        }
        else
        {
            std::cout << mDevicePath << " does not support frame rate adjustments" << std::endl;
            return -1;
        }
    }
    if (mVerbose) {
        std::cout << "Frame rate is " << mFrameRateNumerator << " / " << mFrameRateDenominator << std::endl;
    }

    // Initialize and allocate capture buffers
    rv = initCaptureBuffers();
    if (rv != 0)
    {
        return -1;
    }

    std::cout << "Video device initialized" << std::endl;
    return 0;
}

int VideoSensor::uninitVideoDevice()
{
    releaseCaptureBuffers();
    std::cout << "Video device cleanup complete" << std::endl;
    return 0;
}

int VideoSensor::initCaptureBuffers()
{
    int rv;
   
    // Request buffers
    std::memset(&mRequestBuffers, 0, sizeof(mRequestBuffers));
    mRequestBuffers.count = BUFFER_COUNT;
    mRequestBuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mRequestBuffers.memory = V4L2_MEMORY_MMAP;

    rv = ioctlWait(mFd, VIDIOC_REQBUFS, &mRequestBuffers);
    if (rv != 0)
    {
        std::perror("Failed to request buffers from the video device");
        return -1;
    }

    if (mRequestBuffers.count != BUFFER_COUNT)
    {
        std::perror("Device does not contain valid number of buffers");
        return -1;
    }

    for (unsigned i = 0; i < mRequestBuffers.count; i++)
    {
        struct v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        rv = ioctlWait(mFd, VIDIOC_QUERYBUF, &buf);
        if (rv != 0)
        {
            std::perror("Failed to query buffer");
            return -1;
        }

        mBuffers[i].mLength = buf.length;
        mBuffers[i].mStart = (unsigned char *)mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED, mFd, buf.m.offset);
        if (mBuffers[i].mStart == MAP_FAILED)
        {
            std::perror("Failed to memory map a buffer");
            return -1;
        }
    }

    std::cout << "Video buffers initialized" << std::endl;
    return 0;
}

int VideoSensor::releaseCaptureBuffers()
{
    // Unmap
    for (unsigned i = 0; i < BUFFER_COUNT; i++)
    {
        VideoBuffer * buf = &mBuffers[i];
        if ((buf->mStart != nullptr) || (buf->mStart != MAP_FAILED))
        {
            munmap(buf->mStart, buf->mLength);
            buf->mStart = nullptr;
            buf->mLength = 0;
        }
    }

    std::cout << "Video buffers released" << std::endl;
    return 0;
}

void VideoSensor::pollThread()
{
    while (mPoll)
    {
        int rv = 0;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mFd, &fds);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        rv = select(mFd + 1, &fds, NULL, NULL, &tv);
        if (rv == -1)
        {
            std::perror("Select failed");
            return;
        } 
        else if (rv == 0)
        {
            std::perror("Timeout occured");
            continue;
        }

        // Dequeue the buffer
        struct v4l2_buffer buf;
        std::memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        rv = ioctlWait(mFd, VIDIOC_DQBUF, &buf);
        if (rv != 0)
        {
            std::perror("Failed to dequeue the buffer");
            continue;
        }

        // Process the frame
        rv = process(mBuffers[buf.index].mStart, buf.bytesused);
        if (rv) {
            std::cout << "frame processor error " << rv << std::endl;
        }

        // Queue the buffer
        rv = ioctlWait(mFd, VIDIOC_QBUF, &buf);
        if (rv != 0)
        {
            std::perror("Failed to queue the buffer");
            continue;
        }
    }
}
