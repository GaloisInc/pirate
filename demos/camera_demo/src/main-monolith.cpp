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
#include <string>
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "orientationinputcreator.hpp"
#include "orientationoutputcreator.hpp"
#include "frameprocessorcreator.hpp"
#include "videosourcecreator.hpp"
#include "imageconvert.hpp"
#include "colortracking.hpp"
#include "options.hpp"
#include "optionsparser.hpp"

static std::atomic<bool> interrupted(false);

static int waitInterrupt(void* arg) {
    (void) arg;
    sigset_t set;
    struct signalfd_siginfo unused;
    int fd;

    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);

    fd = signalfd(-1, &set, 0);
    (void)read(fd, &unused, sizeof(unused));

    interrupted = true;
    return 0;
}

int main(int argc, char *argv[])
{
    int rv = 0;
    bool tracing = false;
    Options options;
    sigset_t set;
    struct sigaction newaction;
    std::thread *signalThread = nullptr;
    std::vector<std::shared_ptr<FrameProcessor>> frameProcessors;

    if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
        tracing = true;
    } else {
        ptrace(PTRACE_DETACH, 0, 1, 0);
    }

    if (tracing) {
        // allow SIGINT to continue working under gdb
    } else {
        // block SIGINT for all threads except for the signalThread
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        newaction.sa_handler = SIG_IGN;
        newaction.sa_flags = 0;
        sigemptyset(&newaction.sa_mask);
        sigaction(SIGINT, &newaction, NULL);
    }

    parseArgs(argc, argv, &options);

    OrientationOutput *orientationOutput = nullptr;
    std::vector<std::shared_ptr<OrientationInput>> orientationInputs;
    std::shared_ptr<ColorTracking> colorTracking = nullptr;

    orientationOutput = OrientationOutputCreator::get(options);

    CameraOrientationCallbacks angPosCallbacks = orientationOutput->getCallbacks();
    if (options.mImageTracking) {
        colorTracking = std::make_shared<ColorTracking>(options, angPosCallbacks);
        orientationInputs.push_back(colorTracking);
    }

    if (options.mInputKeyboard) {
        std::shared_ptr<OrientationInput> io =
            std::shared_ptr<OrientationInput>(OrientationInputCreator::get(Keyboard, options, angPosCallbacks));
        orientationInputs.push_back(io);
    }

    if (options.mInputFreespace) {
        std::shared_ptr<OrientationInput> io =
            std::shared_ptr<OrientationInput>(OrientationInputCreator::get(Freespace, options, angPosCallbacks));
        orientationInputs.push_back(io);
    }

    if (options.mFilesystemProcessor) {
        FrameProcessorCreator::add(Filesystem, frameProcessors, options, angPosCallbacks);
    }

    if (options.mXWinProcessor) {
        FrameProcessorCreator::add(XWindows, frameProcessors, options, angPosCallbacks);
    }

    if (options.mH264Encoder) {
        FrameProcessorCreator::add(H264Stream, frameProcessors, options, angPosCallbacks);
    }

    if (options.mImageTracking) {
        // Add color tracking to the end of frame processors.
        // Take advantage of any RGB conversion in previous
        // frame processors.
        frameProcessors.push_back(colorTracking);
    }

    VideoSource *videoSource = VideoSourceCreator::create(options.mVideoInputType, frameProcessors, options);

    rv = orientationOutput->init();
    if (rv != 0)
    {
        goto cleanup;
    }

    for (auto orientationInput : orientationInputs) {
        rv = orientationInput->init();
        if (rv != 0)
        {
            goto cleanup;
        }
    }

    for (auto frameProcessor : frameProcessors) {
        rv = frameProcessor->init();
        if (rv != 0)
        {
            goto cleanup;
        }
    }

    rv = videoSource->init();
    if (rv != 0)
    {
        goto cleanup;
    }

    if (!tracing) {
        signalThread = new std::thread(waitInterrupt, nullptr);
    }

    while (!interrupted)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

cleanup:
    if (signalThread != nullptr) {
        signalThread->join();
        delete signalThread;
    }
    if (videoSource != nullptr) {
        delete videoSource;
    }
    frameProcessors.clear();
    orientationInputs.clear();
    if (orientationOutput != nullptr) {
        delete orientationOutput;
    }
    return rv;
}
