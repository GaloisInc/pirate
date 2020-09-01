#pragma once

#include <string>
#include <X11/Xlib.h>

#include "imageconvert.hpp"
#include "frameprocessor.hpp"
#include "options.hpp"
#include "orientationinput.hpp"
#include "orientationoutput.hpp"

class XWinFrameProcessor : public FrameProcessor
{
public:
    XWinFrameProcessor(const Options& options, OrientationOutput const* orientationOutput, ImageConvert* imageConvert);
    virtual ~XWinFrameProcessor();

    virtual int init() override;
    virtual void term() override;

protected:
    virtual int process(FrameBuffer data, size_t length) override;
    virtual unsigned char* getFrame(unsigned index, VideoType videoType) override;

private:
    OrientationOutput const* mOrientationOutput;
    ImageConvert*            mImageConvert;
    bool                     mMonochrome;
    bool                     mImageSlidingWindow;
    Display*                 mDisplay;
    Window                   mWindow;
    XImage*                  mImage;
    unsigned char*           mImageBuffer;
    unsigned char*           mRGBXImageBuffer;
    unsigned char*           mYUYVImageBuffer;
    GC                       mContext;
    XGCValues                mContextVals;

    int xwinDisplayInitialize();
    void xwinDisplayTerminate();
    int convertJpeg(FrameBuffer data, size_t len);
    int convertYuyv(FrameBuffer data, size_t len);
    void slidingWindow();
    void renderImage();
};

