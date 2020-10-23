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
#include "videosensor.hpp"
#include "videosource.hpp"

#if FFMPEG_PRESENT
#include "mpeg-ts-decoder.hpp"
#endif

class VideoSourceCreator {
public:
    static VideoSource* create(
        VideoType videoInput,
        const std::vector<std::shared_ptr<FrameProcessor>>& frameProcessors,
        const Options& options)
    {
        switch (videoInput)
        {
#if FFMPEG_PRESENT
            case VIDEO_STREAM:
                return new MpegTsDecoder(options, frameProcessors);
#endif
            default:
                return new VideoSensor(options, frameProcessors);
        }
    }
};
