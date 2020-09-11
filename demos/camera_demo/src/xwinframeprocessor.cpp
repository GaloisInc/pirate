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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "xwinframeprocessor.hpp"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

int XWinFrameProcessor::xwinDisplayInitialize() {
    mDisplay = XOpenDisplay(nullptr);
    if (mDisplay == nullptr) {
        std::cout << "Failed to open X display" << std::endl;
        return -1;
    }
    int x_screen = DefaultScreen(mDisplay);
    mWindow = XCreateSimpleWindow(mDisplay,
        RootWindow(mDisplay, x_screen), 10, 10, mImageWidth, mImageHeight, 1,
        BlackPixel(mDisplay, x_screen), WhitePixel(mDisplay, x_screen));
    mContext = XCreateGC(mDisplay, mWindow, 0, &mContextVals);
    mImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);
    if (mImageSlidingWindow) {
        // copy the image without the sliding window
        mRGBXImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 4, 1);
    } else {
        mRGBXImageBuffer = mImageBuffer;
    }
    if (mMonochrome) {
        mYUYVImageBuffer = (unsigned char*) calloc(mImageWidth * mImageHeight * 2, 1);
    } else {
        mYUYVImageBuffer = nullptr;
    }
    if (mMonochrome && (mVideoType != YUYV)) {
            std::cout << "Monochrome filter cannot be used with video type " << mVideoType << std::endl;
            return -1;
    }
    mImage = XCreateImage(mDisplay, CopyFromParent, 24, ZPixmap, 0, (char*) mImageBuffer,
        mImageWidth, mImageHeight, 32, 4 * mImageWidth);
    XMapWindow(mDisplay, mWindow);
    XSync(mDisplay, 0);
    return 0;
}


void XWinFrameProcessor::xwinDisplayTerminate() {
    if (mDisplay != nullptr) {
        XDestroyImage(mImage); // frees mImageBuffer
        XFreeGC(mDisplay, mContext);
        XCloseDisplay(mDisplay);
        if (mImageSlidingWindow) {
            free(mRGBXImageBuffer);
        }
        if (mMonochrome) {
            free(mYUYVImageBuffer);
        }
    }
    mImage = nullptr;
    mDisplay = nullptr;
    mImageBuffer = nullptr;
    mRGBXImageBuffer = nullptr;
    mYUYVImageBuffer = nullptr;
    mContext = nullptr;
}

void XWinFrameProcessor::slidingWindow() {
    int x, y, k;

    std::memcpy(mRGBXImageBuffer, mImageBuffer, mImageHeight * mImageWidth * 4);
    float range = mOrientationOutput->mAngularPositionMax - mOrientationOutput->mAngularPositionMin;
    float position = mOrientationOutput->getAngularPosition();
    float percent = (position - mOrientationOutput->mAngularPositionMin) / range;
    int center = mImageWidth * percent;
    // min can go negative
    int min = (center - mImageWidth / 4);
    int max = (center + mImageWidth / 4);

    for(k = y = 0; y < (int) mImageHeight; y++) {
	    for(x = 0; x < (int) mImageWidth; x++, k += 4) {
            if ((x < min) || (x > max)) {
                mImageBuffer[k+0]=0;
                mImageBuffer[k+1]=0;
                mImageBuffer[k+2]=0;
            }
        }
    }
}

void XWinFrameProcessor::renderImage() {
    int err;

    XPutImage(mDisplay, mWindow, mContext, mImage, 0, 0, 0, 0, mImageWidth, mImageHeight);
    err = errno;
    XFlush(mDisplay);
    errno = err;
}

XWinFrameProcessor::XWinFrameProcessor(const Options& options,
    std::shared_ptr<OrientationOutput> orientationOutput,
    const ImageConvert& imageConvert) :

    FrameProcessor(options.mVideoType, options.mImageWidth, options.mImageHeight),
    mOrientationOutput(orientationOutput),
    mImageConvert(imageConvert),
    mMonochrome(options.mImageMonochrome),
    mImageSlidingWindow(options.mImageSlidingWindow)
{

}

XWinFrameProcessor::~XWinFrameProcessor()
{
    term();
}

int XWinFrameProcessor::init()
{
    return xwinDisplayInitialize();
}

void XWinFrameProcessor::term()
{
    xwinDisplayTerminate();
}

int XWinFrameProcessor::convertYuyv(FrameBuffer data, size_t length) {
    if (mMonochrome) {
        memcpy(mYUYVImageBuffer, data, length);
        for (size_t src = 0; src < length; src++) {
            if ((src % 2) == 1) {
                mYUYVImageBuffer[src] = 128;
            }
        }
        return mImageConvert.convert(mYUYVImageBuffer, length, YUYV, mImageBuffer, RGBX);
    } else {
        return mImageConvert.convert(data, length, YUYV, mImageBuffer, RGBX);
    }
}

int XWinFrameProcessor::convertJpeg(FrameBuffer data, size_t length) {
    return mImageConvert.convert(data, length, JPEG, mImageBuffer, RGBX);
}

int XWinFrameProcessor::process(FrameBuffer data, size_t length)
{
    int rv;
    switch (mVideoType) {
        case JPEG:
            rv = convertJpeg(data, length);
            break;
        case YUYV:
            rv = convertYuyv(data, length);
            break;
        default:
            std::cout << "Unknown video type " << mVideoType << std::endl;
            return -1;
    }
    if (rv) {
        return rv;
    }
    if (mImageSlidingWindow) {
        slidingWindow();
    }
    renderImage();
    return 0;
}

unsigned char* XWinFrameProcessor::getFrame(unsigned index, VideoType videoType) {
    if (index != mIndex) {
        return nullptr;
    }
    if (videoType == RGBX) {
        return mRGBXImageBuffer;
    } else {
        return nullptr;
    }
}
