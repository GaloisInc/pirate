/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#pragma once

#include "options.hpp"

#include "orientationinput.hpp"
#include "orientationoutput.hpp"
#include "fileframeprocessor.hpp"
#include "videosource.hpp"

#if XWIN_PRESENT
#include "xwinframeprocessor.hpp"
#endif

#if FFMPEG_PRESENT
#include "h264encoder.hpp"
#endif

class FrameProcessorCreator {
public:
    static void add(
        FrameProcessorType processorType,
        std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors,
        const Options& options,
        std::shared_ptr<OrientationOutput> orientationOutput)
    {
        (void) orientationOutput;

        FrameProcessor *fp = nullptr;

        switch (processorType)
        {
#if XWIN_PRESENT
            case XWindows:
                fp = new XWinFrameProcessor(options, orientationOutput);
                break;
#endif
#if FFMPEG_PRESENT
            case H264Stream:
                fp = new H264Encoder(options);
                break;
#endif
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
