#pragma once

#include "options.hpp"

#include "fileframeprocessor.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

class FrameProcessorCreator {
public:
    static FrameProcessor * get(Options& options)
    {
        switch (options.mProcessorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(options.mVideoType,
                   options.mImageWidth, options.mImageHeight,
                   options.mImageMonochrome);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(options.mVideoType,
                    options.mImageOutputDirectory, options.mVerbose);
        }
    }
};
