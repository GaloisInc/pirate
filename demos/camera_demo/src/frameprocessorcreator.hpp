#pragma once

#include "fileframeprocessor.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

class FrameProcessorCreator {
public:
    static FrameProcessor * get(FrameProcessorType processorType,
        VideoType videoType, unsigned width, unsigned height,
        bool monochrome, std::string outdir, bool verbose)
    {
        switch (processorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(videoType, width, height, monochrome);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(videoType, outdir, verbose);
        }
    }
};
