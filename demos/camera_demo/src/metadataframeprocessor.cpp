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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <sys/time.h>

#include "metadataframeprocessor.hpp"

#include "binary-us-map.h"

#include "orion-sdk/KlvParser.hpp"
#include "orion-sdk/KlvTree.hpp"

MetaDataFrameProcessor::MetaDataFrameProcessor(const Options& options) :
    FrameProcessor(options.mVideoOutputType, options.mImageWidth, options.mImageHeight),
    mDisplay(nullptr),
    mImage(nullptr),
    mImageBuffer(nullptr),
    mMapWidth(550),
    mMapHeight(300),
    mMapBuffer(nullptr),
    mSquareWidth(16),
    mSquareHeight(16),
    mLatitude(0.0),
    mLongitude(0.0),
    mTimestampMillis(0)
{

}

MetaDataFrameProcessor::~MetaDataFrameProcessor()
{
    term();
}

int MetaDataFrameProcessor::init()
{
    ImageConvert mapImageConvert(mMapWidth, mMapHeight);
    unsigned char *mapBuffer;
    size_t mapLength = 0;
    int rv;

    mDisplay = XOpenDisplay(nullptr);
    if (mDisplay == nullptr) {
        std::cout << "Failed to open X display" << std::endl;
        return -1;
    }
    int x_screen = DefaultScreen(mDisplay);
    mWindow = XCreateSimpleWindow(mDisplay,
        RootWindow(mDisplay, x_screen), 10, 10, mMapWidth, mMapHeight, 1,
        BlackPixel(mDisplay, x_screen), WhitePixel(mDisplay, x_screen));
    mContext = XCreateGC(mDisplay, mWindow, 0, &mContextVals);
    mImageBuffer = (unsigned char*) calloc(mMapWidth * mMapHeight * 4, 1);
    mImage = XCreateImage(mDisplay, CopyFromParent, 24, ZPixmap, 0, (char*) mImageBuffer,
        mMapWidth, mMapHeight, 32, 4 * mMapWidth);
    XMapWindow(mDisplay, mWindow);
    XSync(mDisplay, 0);

    rv = mapImageConvert.convert(USA_Mercator_jpg, USA_Mercator_jpg_len, VIDEO_JPEG, VIDEO_BGRX, 1);
    if (rv) {
        return rv;
    }
    mapBuffer = mapImageConvert.getBuffer(VIDEO_BGRX, 1, &mapLength);
    mMapBuffer = (unsigned char*) calloc(mapLength, 1);
    std::memcpy(mMapBuffer, mapBuffer, mapLength);
    return 0;
}

void MetaDataFrameProcessor::renderImage() {
    int err;

    XPutImage(mDisplay, mWindow, mContext, mImage, 0, 0, 0, 0, mMapWidth, mMapHeight);
    err = errno;
    XFlush(mDisplay);
    errno = err;
}

void MetaDataFrameProcessor::term()
{
    if (mDisplay != nullptr) {
        XDestroyImage(mImage); // frees mImageBuffer
        XFreeGC(mDisplay, mContext);
        XCloseDisplay(mDisplay);
    }
    if (mMapBuffer != nullptr) {
        free(mMapBuffer);
    }
    mImage = nullptr;
    mDisplay = nullptr;
    mImageBuffer = nullptr;
    mContext = nullptr;
}

void MetaDataFrameProcessor::paintSquare(int xCenter, int yCenter)
{
    int k, x, y;
    int xMin = xCenter - (mSquareWidth / 2);
    int xMax = xCenter + (mSquareWidth / 2);
    int yMin = yCenter - (mSquareHeight / 2);
    int yMax = yCenter + (mSquareHeight / 2);

    for(k = y = 0; y < (int) mMapHeight; y++) {
	    for(x = 0; x < (int) mMapWidth; x++, k += 4) {
            if ((x > xMin) && (x < xMax) && (y > yMin) && (y < yMax)) {
                mImageBuffer[k+0]=255;
                mImageBuffer[k+1]=0;
                mImageBuffer[k+2]=0;
            }
        }
    }
}

static const float mapLonLeft = -124.0;
static const float mapLonRight = -67.0;
static const float mapLatBottom = 25.0;
static const float mapLatBottomRad = mapLatBottom * M_PI / 180.0;
static const float mapLonDelta = mapLonRight - mapLonLeft;

void MetaDataFrameProcessor::toMercatorProjection(float lat, float lon, int& x, int& y) {
    x = (int) ((lon - mapLonLeft) * (mMapWidth / mapLonDelta));
    float latRad = (lat * M_PI) / 180.0;
    float worldMapWidth = ((mMapWidth / mapLonDelta) * 360.0) / (2.0 * M_PI);
    float mapOffsetY = (worldMapWidth / 2.0) * log((1.0 + sin(mapLatBottomRad)) / (1.0 - sin(mapLatBottomRad)));
    y = mMapHeight - ((int) ((worldMapWidth / 2.0 * log((1.0 + sin(latRad)) / (1.0 - sin(latRad)))) - mapOffsetY));
}

int MetaDataFrameProcessor::process(FrameBuffer data, size_t length, DataStreamType dataStream)
{
    int xCenter, yCenter;
    struct timeval tv;
    uint64_t nowMillis;

    gettimeofday(&tv, nullptr);
    nowMillis = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if (dataStream == MetaData) {
        int success1, success2;
        double lat, lon;
        KlvNewData(data, length);
        lat = KlvGetValueDouble(KLV_UAS_SENSOR_LAT, &success1);
        lon = KlvGetValueDouble(KLV_UAS_SENSOR_LON, &success2);
        if (success1 && success2) {
            mLatitude = lat * 180.0 / M_PI;
            mLongitude = lon * 180.0 / M_PI;
            mTimestampMillis = nowMillis;
        }
    }

    std::memcpy(mImageBuffer, mMapBuffer, mMapWidth * mMapHeight * 4);

    // show the location if it has been updated within the past 1000 milliseconds
    if ((nowMillis - mTimestampMillis) < 1000) {
        toMercatorProjection(mLatitude, mLongitude, xCenter, yCenter);
        paintSquare(xCenter, yCenter);
    }

    renderImage();

    return 0;
}

