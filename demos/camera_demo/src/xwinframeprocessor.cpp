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

XWinFrameProcessor::XWinFrameProcessor(const Options& options,
    CameraOrientationCallbacks angPosCallbacks) :

    FrameProcessor(VIDEO_BGRX, options.mImageWidth, options.mImageHeight),
    mCallbacks(angPosCallbacks),
    mPanAxisMin(options.mPanAxisMin),
    mPanAxisMax(options.mPanAxisMax),
    mTiltAxisMin(options.mTiltAxisMin),
    mTiltAxisMax(options.mTiltAxisMax),
    mImageSlidingWindow(options.mImageSlidingWindow),
    mDisplay(nullptr),
    mImage(nullptr),
    mImageBuffer(nullptr)
{

}

XWinFrameProcessor::~XWinFrameProcessor()
{
    term();
}

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
    }
    mImage = nullptr;
    mDisplay = nullptr;
    mImageBuffer = nullptr;
    mContext = nullptr;
}

void XWinFrameProcessor::slidingWindow() {
    int x, y, k;

    float x_range = mPanAxisMax - mPanAxisMin;
    float y_range = mTiltAxisMax - mTiltAxisMin;
    PanTilt position = mCallbacks.mGet();
    float x_percent = (position.pan - mPanAxisMin) / x_range;
    float y_percent = (-position.tilt - mTiltAxisMin) / y_range;
    int x_center = mImageWidth * x_percent;
    int y_center = mImageHeight * y_percent;
    // min can go negative
    int x_min = (x_center - mImageWidth / 3);
    int x_max = (x_center + mImageWidth / 3);
    int y_min = (y_center - mImageHeight / 3);
    int y_max = (y_center + mImageHeight / 3);

    for(k = y = 0; y < (int) mImageHeight; y++) {
	    for(x = 0; x < (int) mImageWidth; x++, k += 4) {
            if ((x < x_min) || (x > x_max) || (y < y_min) || (y > y_max)) {
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

int XWinFrameProcessor::init()
{
    return xwinDisplayInitialize();
}

void XWinFrameProcessor::term()
{
    xwinDisplayTerminate();
}


int XWinFrameProcessor::process(FrameBuffer data, size_t length, DataStreamType dataStream)
{
    if (dataStream != VideoData) {
        return 0;
    }
    if (length != (mImageWidth * mImageHeight * 4)) {
        std::cout << "xwindows unexpected frame length " << length << std::endl;
        return 1;
    }
    memcpy(mImageBuffer, data, length);
    if (mImageSlidingWindow) {
        slidingWindow();
    }
    renderImage();
    return 0;
}
