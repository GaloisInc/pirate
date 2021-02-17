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

#include "cameracontrolinputcreator.hpp"
#include "cameracontroloutputcreator.hpp"
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
    Options options;
    sigset_t set;
    struct sigaction newaction;
    std::thread *signalThread = nullptr;
    std::vector<std::shared_ptr<FrameProcessor>> frameProcessors;
 
    parseArgs(argc, argv, &options);

    if (!options.mGDB) {
        // block SIGINT for all threads except for the signalThread
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        pthread_sigmask(SIG_BLOCK, &set, NULL);
        newaction.sa_handler = SIG_IGN;
        newaction.sa_flags = 0;
        sigemptyset(&newaction.sa_mask);
        sigaction(SIGINT, &newaction, NULL);
    }

    CameraControlOutput *cameraControlOutput = nullptr;
    std::vector<std::shared_ptr<CameraControlInput>> cameraControlInputs;
    std::shared_ptr<ColorTracking> colorTracking = nullptr;

    cameraControlOutput = CameraControlOutputCreator::get(options);

    CameraControlCallbacks cameraControlCallbacks = cameraControlOutput->getCallbacks();
    if (options.mImageTracking) {
        colorTracking = std::make_shared<ColorTracking>(options, cameraControlCallbacks);
        cameraControlInputs.push_back(colorTracking);
    }

    if (options.mInputKeyboard) {
        std::shared_ptr<CameraControlInput> io =
            std::shared_ptr<CameraControlInput>(CameraControlInputCreator::get(Keyboard, options, cameraControlCallbacks));
        cameraControlInputs.push_back(io);
    }

    if (options.mInputFreespace) {
        std::shared_ptr<CameraControlInput> io =
            std::shared_ptr<CameraControlInput>(CameraControlInputCreator::get(Freespace, options, cameraControlCallbacks));
        cameraControlInputs.push_back(io);
    }

    if (options.mFilesystemProcessor) {
        FrameProcessorCreator::add(Filesystem, frameProcessors, options, cameraControlCallbacks);
    }

    if (options.mXWinProcessor) {
        FrameProcessorCreator::add(XWindows, frameProcessors, options, cameraControlCallbacks);
    }

    if (options.mMetaDataProcessor) {
        FrameProcessorCreator::add(MetaDataProcessor, frameProcessors, options, cameraControlCallbacks);
    }

    if (options.mH264Encoder) {
        FrameProcessorCreator::add(H264Stream, frameProcessors, options, cameraControlCallbacks);
    }

    if (options.mImageTracking) {
        // Add color tracking to the end of frame processors.
        // Take advantage of any RGB conversion in previous
        // frame processors.
        frameProcessors.push_back(colorTracking);
    }

    VideoSource *videoSource = VideoSourceCreator::create(options.mVideoInputType, frameProcessors, options);

    rv = cameraControlOutput->init();
    if (rv != 0)
    {
        goto cleanup;
    }

    for (auto cameraControlInput : cameraControlInputs) {
        rv = cameraControlInput->init();
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

    if (!options.mGDB) {
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
    cameraControlInputs.clear();
    if (cameraControlOutput != nullptr) {
        delete cameraControlOutput;
    }
    return rv;
}
