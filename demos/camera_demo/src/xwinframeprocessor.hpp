#pragma once

#include <string>
#include <X11/Xlib.h>

#include "frameprocessor.hpp"
#include "options.hpp"
#include "orientationoutput.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(const Options& options, OrientationOutput const* orientationOutput);
    virtual ~XWinFrameProcessor();

    virtual int init();
    virtual void term();
    virtual int processFrame(FrameBuffer data, size_t length);

private:
    OrientationOutput const* mOrientationOutput;
    unsigned                 mImageWidth;
    unsigned                 mImageHeight;
    bool                     mMonochrome;
    bool                     mImageSlidingWindow;
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
    void slidingWindow();
    void renderImage();
};

