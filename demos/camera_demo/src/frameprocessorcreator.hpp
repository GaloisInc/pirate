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
    static FrameProcessor * get(FrameProcessorType processorType,
        const Options& options,
        std::shared_ptr<OrientationOutput> orientationOutput,
        const ImageConvert& imageConvert)
    {
        (void) orientationOutput;
        (void) imageConvert;
        switch (processorType)
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
