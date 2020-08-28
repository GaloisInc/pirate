#pragma once

#include "options.hpp"

#include "orientationoutput.hpp"
#include "fileframeprocessor.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

class FrameProcessorCreator {
public:
    static FrameProcessor * get(const Options& options, OrientationOutput* orientationOutput)
    {
        (void) orientationOutput;
        switch (options.mProcessorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(options, orientationOutput);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(options.mVideoType,
                    options.mImageOutputDirectory, options.mVerbose);
        }
    }
};
