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
    static FrameProcessor * get(const Options& options,
        OrientationOutput const* orientationOutput,
        const ImageConvert& imageConvert)
    {
        (void) orientationOutput;
        (void) imageConvert;
        switch (options.mProcessorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(options, orientationOutput, imageConvert);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(options);
        }
    }
};
