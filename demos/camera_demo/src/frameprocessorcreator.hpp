#pragma once

#include "options.hpp"

#include "orientationinput.hpp"
#include "orientationoutput.hpp"
#include "fileframeprocessor.hpp"

#include "h264streamer.hpp"

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
            case H264:
                return new H264Streamer(options);
            case Filesystem:
            default:
                return new FileFrameProcessor(options);
        }
    }
};
