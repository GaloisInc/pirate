#pragma once

#include "options.hpp"

#include "orientationinput.hpp"
#include "orientationoutput.hpp"
#include "fileframeprocessor.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

class FrameProcessorCreator {
public:
    static FrameProcessor * get(const Options& options, OrientationInput* orientationInput, OrientationOutput const* orientationOutput)
    {
        (void) orientationInput;
        (void) orientationOutput;
        switch (options.mProcessorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(options, orientationInput, orientationOutput);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(options.mVideoType,
                    options.mImageOutputDirectory, options.mVerbose);
        }
    }
};
