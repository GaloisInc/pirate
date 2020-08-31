#pragma once

#include <string>
#include <X11/Xlib.h>

#include "frameprocessor.hpp"
#include "options.hpp"
#include "orientationinput.hpp"
#include "orientationoutput.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(const Options& options, OrientationInput* orientationInput, OrientationOutput const* orientationOutput);
    virtual ~XWinFrameProcessor();

    virtual int init();
    virtual void term();
    virtual int processFrame(FrameBuffer data, size_t length);

private:
    OrientationInput*        mOrientationInput;
    OrientationOutput const* mOrientationOutput;
    unsigned                 mImageWidth;
    unsigned                 mImageHeight;
    bool                     mMonochrome;
    bool                     mImageSlidingWindow;
    unsigned char            mImageTrackingRGB[3];
    uint64_t                 mImageTrackingFrameCount;
    Display*                 mDisplay;
    Window                   mWindow;
    XImage*                  mImage;
    unsigned char*           mImageBuffer;
    unsigned char*           mTempImageBuffer;
    unsigned char*           mTempImageBufferRow;
    GC                       mContext;
    XGCValues                mContextVals;

    int xwinDisplayInitialize();
    void xwinDisplayTerminate();
    int convertJpeg(FrameBuffer buf, size_t len);
    int convertYuyv(FrameBuffer buf, size_t len);
    int computeTrackingRGB();
    void trackRGB();
    void slidingWindow();
    void renderImage();
};

