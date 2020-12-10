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

#pragma once

#include <string>
#include <X11/Xlib.h>

#include "frameprocessor.hpp"
#include "imageconvert.hpp"

class MetaDataFrameProcessor : public FrameProcessor
{
public:
    MetaDataFrameProcessor(const Options& options);
    virtual ~MetaDataFrameProcessor();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int process(FrameBuffer data, size_t length, DataStreamType dataStream) override;

private:
    Display*                   mDisplay;
    Window                     mWindow;
    XImage*                    mImage;
    unsigned char*             mImageBuffer;
    GC                         mContext;
    XGCValues                  mContextVals;

    const unsigned  mMapWidth;
    const unsigned  mMapHeight;
    unsigned char*  mMapBuffer;

    const unsigned  mSquareWidth;
    const unsigned  mSquareHeight;

    float mLatitude, mLongitude;
    uint64_t mTimestampMillis;

    void renderImage();
    void toMercatorProjection(float lat, float lon, int& x, int& y);
    void paintSquare(int xCenter, int yCenter);
};

