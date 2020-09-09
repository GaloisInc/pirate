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
    static void add(
        std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors,
        FrameProcessorType processorType,
        const Options& options,
        std::shared_ptr<OrientationOutput> orientationOutput,
        const ImageConvert& imageConvert)
    {
        (void) orientationOutput;
        (void) imageConvert;

        FrameProcessor *fp = nullptr;

        switch (processorType)
        {
#if XWIN_PRESENT
            case XWindows:
                fp = new XWinFrameProcessor(options, orientationOutput, imageConvert);
                break;
#endif
            case H264Stream:
                fp = new H264Streamer(options);
                break;
            case Filesystem:
                fp = new FileFrameProcessor(options);
                break;
            default:
                std::cout << "Skipping unknown frame processor." << std::endl;
                break;
        }
        if (fp != nullptr)
        {
            std::shared_ptr<FrameProcessor> frameProcessor = std::shared_ptr<FrameProcessor>(fp);
            frameProcessors.push_back(frameProcessor);
        }
    }
};
