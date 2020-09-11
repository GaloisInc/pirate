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

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <argp.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "orientationinputcreator.hpp"
#include "orientationoutputcreator.hpp"
#include "frameprocessorcreator.hpp"
#include "imageconvert.hpp"
#include "colortracking.hpp"
#include "videosensor.hpp"
#include "options.hpp"

const int OPT_THRESH   = 129;
const int OPT_OUT_DIR  = 130;
const int OPT_MAX_OUT  = 131;
const int OPT_MONO     = 132;
const int OPT_SLIDE    = 133;
const int OPT_LIMIT    = 134;

static struct argp_option options[] =
{
    { 0,              0,            0,             0, "video options:",                           1 },
    { "video_device", 'd',          "device",      0, "video device",                             0 },
    { "video_type",   't',          "type",        0, "video type (jpeg|yuyv|h264)",              0 },
    { "width",        'W',          "pixels",      0, "image width",                              0 },
    { "height",       'H',          "pixels",      0, "image height",                             0 },
    { "flip",         'f',          "v|h",         0, "horizontal or vertical image flip",        0 },
    { 0,              0,            0,             0, "frame processor options:",                 2 },
    { "color_track",  'C',          "RRGGBB",      0, "color tracking (RGB hex)",                 0 },
    { "threshold",    OPT_THRESH,   "val",         0, "color tracking threshold",                 0 },
    { "xwindows",     'X',          NULL,          0, "xwindows frame processor",                 0 },
    { "filesystem",   'F',          NULL,          0, "filesystem frame processor",               0 },
    { "encoder",      'E',          "url",         0, "MPEG-TS H.264 encoder url (host:port)",    0 },
    { "out_dir",      OPT_OUT_DIR,  "path",        0, "image output directory",                   0 },
    { "out_count",    OPT_MAX_OUT,  "val",         0, "image output maximum file count",          0 },
    { "monochrome",   OPT_MONO,     NULL,          0, "monochrome image filter",                  0 },
    { "sliding",      OPT_SLIDE,    NULL,          0, "sliding window image filter",              0 },
    { 0,              0,            0,             0, "input/output options:",                    3 },
    { "input",        'i',          "acc|kbd",     0, "position input",                           0 },
    { "output",       'o',          "servo|print", 0, "angular position output",                  0 },
    { "output_limit", OPT_LIMIT,    "val",         0, "angular position bound",                   0 },
    { "verbose",      'v',          NULL,          0, "verbose output",                           4 },
    { NULL,            0 ,          NULL,          0, NULL,                                       0 },
};

static std::atomic<bool> interrupted(false);

static error_t parseOpt(int key, char * arg, struct argp_state * state)
{
    Options * opt = static_cast<Options *>(state->input);
    std::istringstream ss(arg != NULL ? arg : "");

    switch (key)
    {
        case 'd':
            ss >> opt->mVideoDevice;
            break;

        case 't':
            if (ss.str() == "jpeg")
            {
                opt->mVideoType = JPEG;
            }
            else if (ss.str() == "yuyv")
            {
                opt->mVideoType = YUYV;
            }
            else if (ss.str() == "h264")
            {
                opt->mVideoType = H264;
            }
            else
            {
                argp_usage(state);
                argp_error(state, "invalid video type argument '%s'", arg);
            }
            break;

        case 'W':
            ss >> opt->mImageWidth;
            break;

        case 'H':
            ss >> opt->mImageHeight;
            break;

        case 'f':
            if (ss.str() == "v")
            {
                opt->mImageVerticalFlip = true;
            }
            else if (ss.str() == "h")
            {
                opt->mImageHorizontalFlip = true;
            }
            else
            {
                argp_error(state, "invalid flip argument '%s'", arg);
            }

            break;

        case OPT_OUT_DIR:
            ss >> opt->mImageOutputDirectory;
            break;

        case OPT_MAX_OUT:
            ss >> opt->mImageOutputMaxFiles;
            break;

        case 'o':
            if (ss.str() == "servo")
            {
                opt->mOutputType = PiServo;
            }
            else if (ss.str() == "print")
            {
                opt->mOutputType = Print;
            }
            else
            {
                argp_error(state, "invalid output argument '%s'", arg);
            }
            break;

        case 'i':
            if (ss.str() == "acc")
            {
                opt->mInputType = Freespace;
            }
            else if (ss.str() == "kbd")
            {
                opt->mInputType = Keyboard;
            }
            else
            {
                argp_error(state, "invalid input argument '%s'", arg);
            }
            break;

        case 'X':
            opt->mXWinProcessor = true;
            break;

        case 'F':
            opt->mFilesystemProcessor = true;
            break;

        case 'E':
            opt->mH264Encoder = true;
            ss >> opt->mH264Url;
            if (opt->mH264Url.find(':') == std::string::npos)
            {
                argp_error(state, "mpeg-ts encoder argument '%s' must be host:port", arg);
            }
            if (opt->mH264Url.find("udp://") != std::string::npos)
            {
                // do nothing
            }
            else if (opt->mH264Url.find("://") != std::string::npos)
            {
                argp_error(state, "mpeg-ts encoder argument '%s' must be host:port", arg);
            }
            else
            {
                opt->mH264Url = "udp://" + opt->mH264Url;
            }
            break;

        case OPT_LIMIT:
            ss >> opt->mAngularPositionLimit;
            break;

        case OPT_MONO:
            opt->mImageMonochrome = true;
            break;

        case OPT_SLIDE:
            opt->mImageSlidingWindow = true;
            break;

        case 'C': {
            std::string argval = ss.str();
            opt->mImageTracking = true;
            if (argval.length() != 6)
            {
                argp_error(state, "invalid length of color tracking argument '%s'", arg);
            }
            for (size_t i = 0; i < 6; i++)
            {
                size_t idx = i / 2;
                size_t shift = ((i + 1) % 2);
                char c = argval[i];
                int val = -1;
                if ((c >= '0') && (c <= '9')) {
                    val = c - '0';
                } else if (c >= 'a' && c <= 'f') {
                    val = c - 'a' + 10;
                } else if (c >= 'A' && c <= 'F') {
                    val = c - 'A' + 10;
                }
                if (val < 0) {
                    argp_error(state, "invalid RRGGBB color tracking argument '%s'", arg);
                } else {
                    opt->mImageTrackingRGB[idx] += (val) << (shift ? 4 : 0);
                }
            }
            break;
        }

        case OPT_THRESH:
            ss >> opt->mImageTrackingThreshold;
            break;

        case 'v':
            opt->mVerbose = true;
            break;
    }

    return 0;
}

static int waitInterrupt(void* arg) {
    (void) arg;
    sigset_t set;
    struct signalfd_siginfo unused;
    int fd;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    fd = signalfd(-1, &set, 0);
    read(fd, &unused, sizeof(unused));

    interrupted = true;
    return 0;
}

static void parseArgs(int argc, char * argv[], Options * opt)
{
    struct argp argp;
    argp.options = options;
    argp.parser = parseOpt;
    argp.args_doc = "test";
    argp.doc = "Embedded application based on camera, position input and position driver";
    argp.children = NULL;
    argp.help_filter = NULL;
    argp.argp_domain = NULL;

    argp_parse(&argp, argc, argv, 0, 0, opt);
}

int main(int argc, char *argv[])
{
    int rv;
    Options options;
    sigset_t set;
    std::thread *signalThread;
    std::vector<std::shared_ptr<FrameProcessor>> frameProcessors;

    // block SIGINT for all threads except for the signalThread
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    parseArgs(argc, argv, &options);

    ImageConvert imageConvert(options.mImageWidth, options.mImageHeight);
    std::shared_ptr<OrientationOutput> orientationOutput;
    std::shared_ptr<OrientationInput> orientationInput;
    std::shared_ptr<ColorTracking> colorTracking = nullptr;

    orientationOutput = std::shared_ptr<OrientationOutput>(OrientationOutputCreator::get(options));

    if (options.mImageTracking) {
        colorTracking = std::make_shared<ColorTracking>(options, orientationOutput->getUpdateCallback());
        orientationInput = colorTracking;
    } else {
        OrientationInput* oi = OrientationInputCreator::get(options, orientationOutput->getUpdateCallback());
        orientationInput = std::shared_ptr<OrientationInput>(oi);
    }

    if (options.mFilesystemProcessor) {
        FrameProcessorCreator::add(frameProcessors, Filesystem, options, orientationOutput, imageConvert);
    }

    if (options.mXWinProcessor) {
        FrameProcessorCreator::add(frameProcessors, XWindows, options, orientationOutput, imageConvert);
    }

    if (options.mH264Encoder) {
        FrameProcessorCreator::add(frameProcessors, H264Stream, options, orientationOutput, imageConvert);
    }

    if (options.mImageTracking) {
        // Add color tracking to the end of frame processors.
        // Take advantage of any RGB conversion in previous
        // frame processors.
        frameProcessors.push_back(colorTracking);
    }

    VideoSensor *videoSensor = new VideoSensor(options, frameProcessors, imageConvert);

    rv = orientationOutput->init();
    if (rv != 0)
    {
        return -1;
    }

    rv = orientationInput->init();
    if (rv != 0)
    {
        return -1;
    }

    for (auto frameProcessor : frameProcessors) {
        rv = frameProcessor->init();
        if (rv != 0)
        {
            return -1;
        }
    }

    rv = videoSensor->init();
    if (rv != 0)
    {
        videoSensor->term();
        return -1;
    }

    signalThread = new std::thread(waitInterrupt, nullptr);

    while (!interrupted)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    signalThread->join();

    delete signalThread;
    delete videoSensor;
    frameProcessors.clear();
    orientationInput = nullptr;
    orientationOutput = nullptr;

    return 0;
}
