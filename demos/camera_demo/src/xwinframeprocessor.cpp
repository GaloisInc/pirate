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
    std::shared_ptr<OrientationOutput> orientationOutput) :

    FrameProcessor(BGRX, options.mImageWidth, options.mImageHeight),
    mOrientationOutput(orientationOutput),
    mImageSlidingWindow(options.mImageSlidingWindow)
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

int XWinFrameProcessor::init()
{
    return xwinDisplayInitialize();
}

void XWinFrameProcessor::term()
{
    xwinDisplayTerminate();
}


int XWinFrameProcessor::process(FrameBuffer data, size_t length)
{
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
