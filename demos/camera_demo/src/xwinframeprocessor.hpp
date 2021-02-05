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

#include <memory>
#include <string>
#include <X11/Xlib.h>

#include "imageconvert.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"
#include "cameracontrolinput.hpp"
#include "cameracontroloutput.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(const Options& options,
        CameraControlCallbacks cameraControlCallbacks);
    virtual ~XWinFrameProcessor();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int process(FrameBuffer data, size_t length, DataStreamType dataStream) override;

private:
    CameraControlCallbacks mCallbacks;
    const float            mPanAxisMin;
    const float            mPanAxisMax;
    const float            mTiltAxisMin;
    const float            mTiltAxisMax;
    const bool             mColorPick;
    bool                   mImageSlidingWindow;
    Display*               mDisplay;
    Window                 mWindow;
    XImage*                mImage;
    unsigned char*         mImageBuffer;
    GC                     mContext;
    XGCValues              mContextVals;
    static const uint32_t  mColorPickBoxSize = 50;
    static const uint32_t  mColorPickBoxColor = 0x00FF4500; // red

    int xwinDisplayInitialize();
    void xwinDisplayTerminate();
    void slidingWindow();
    void renderImage();
    void colorPick();
};

