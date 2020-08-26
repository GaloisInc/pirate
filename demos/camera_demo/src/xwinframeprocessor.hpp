#pragma once

#include <string>
#include <X11/Xlib.h>
#include "frameprocessor.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(VideoType videoType, unsigned width, unsigned height, bool monochrome);
    virtual ~XWinFrameProcessor();

    virtual int init();
    virtual void term();
    virtual int processFrame(FrameBuffer data, size_t length);

private:
    unsigned       mImageWidth;
    unsigned       mImageHeight;
    bool           mMonochrome;
    Display*       mDisplay;
    Window         mWindow;
    XImage*        mImage;
    unsigned char* mImageBuffer;
    unsigned char* mTempImageBuffer;
    unsigned char* mTempImageBufferRow;
    GC             mContext;
    XGCValues      mContextVals;

    int xwinDisplayInitialize();
    void xwinDisplayTerminate();
    int convertJpeg(FrameBuffer buf, size_t len);
    int convertYuyv(FrameBuffer buf, size_t len);
    void renderImage();
};

