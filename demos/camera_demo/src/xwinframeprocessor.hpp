#pragma once

#include <string>
#include <X11/Xlib.h>
#include "frameprocessor.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(unsigned width, unsigned height);
    virtual ~XWinFrameProcessor();

    virtual int init();
    virtual void term();
    virtual int processFrame(FrameBuffer data, size_t length);

private:
    unsigned       mImageWidth;
    unsigned       mImageHeight;
    Display*       mDisplay;
    Window         mWindow;
    XImage*        mImage;
    char*          mImageBuffer;
    unsigned char* mJpegBuffer;
    unsigned char* mJpegBufferRow;
    GC             mContext;
    XGCValues      mContextVals;

    int xwinDisplayInitialize();
    void xwinDisplayTerminate();
    void convertJpeg(FrameBuffer buf, size_t len);
    void renderImage();
};

