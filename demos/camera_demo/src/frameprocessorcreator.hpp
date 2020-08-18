#pragma once

#include "fileframeprocessor.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

class FrameProcessorCreator {
public:
    enum FrameProcessorType { Filesystem, XWindows };

    static FrameProcessor * get(FrameProcessorType processorType,
        unsigned width, unsigned height, std::string outdir, bool verbose)
    {
        switch (processorType)
        {
#if XWIN_PRESENT
            case XWindows:
                return new XWinFrameProcessor(width, height);
#endif
            case Filesystem:
            default:
                return new FileFrameProcessor(outdir, verbose);
        }
    }
};
