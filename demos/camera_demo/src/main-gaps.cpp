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
#include <thread>
#include <vector>

#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "libpirate.h"

#include "orientationinputcreator.hpp"
#include "orientationoutputcreator.hpp"
#include "remoteorientationoutput.hpp"
#include "frameprocessorcreator.hpp"
#include "videosourcecreator.hpp"
#include "imageconvert.hpp"
#include "colortracking.hpp"
#include "options.hpp"
#include "optionsparser.hpp"
#include "remotes.hpp"

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

void pirateInitReaders(RemoteDescriptors &remotes, const Options &options, int &success) {
    int rv;
    success = -1;

    rv = pirate_open_parse(options.mOutputServerChannel.c_str(), O_RDONLY);
    if (rv < 0) {
        perror("pirate open server write channel error");
        return;
    }
    remotes.mServerReadGd = rv;
    rv = pirate_open_parse(options.mOutputClientChannel.c_str(), O_RDONLY);
    if (rv < 0) {
        perror("pirate open client read channel error");
        return;
    }
    remotes.mClientReadGd = rv;

    success = 0;
}

void pirateInitWriters(RemoteDescriptors &remotes, Options &options, int &success) {
    int rv;
    success = -1;

    rv = pirate_open_parse(options.mOutputServerChannel.c_str(), O_WRONLY);
    if (rv < 0) {
        perror("pirate open server write channel error");
        return;
    }
    remotes.mClientWriteGd = rv;

    rv = pirate_open_parse(options.mOutputClientChannel.c_str(), O_WRONLY);
    if (rv < 0) {
        perror("pirate open client write channel error");
        return;
    }
    remotes.mServerWriteGds.push_back(rv);

    success = 0;
}

void pirateCloseRemotes(const RemoteDescriptors &remotes) {
    if (remotes.mClientReadGd > 0) {
        pirate_close(remotes.mClientReadGd);
    }
    if (remotes.mClientWriteGd > 0) {
        pirate_close(remotes.mClientWriteGd);       
    }
    if (remotes.mServerReadGd > 0) {
        pirate_close(remotes.mServerReadGd);
    }
    for (auto gd : remotes.mServerWriteGds) {
        if (gd > 0) {
            pirate_close(gd);
        }
    }
}

int main(int argc, char *argv[])
{
    int rv, readerSuccess, writerSuccess;
    Options options;
    RemoteDescriptors remotes;
    sigset_t set;
    std::thread *signalThread, *readerInitThread, *writerInitThread;
    std::vector<std::shared_ptr<FrameProcessor>> frameProcessors;

    parseArgs(argc, argv, &options);
    readerInitThread = new std::thread(pirateInitReaders, std::ref(remotes), std::ref(options), std::ref(readerSuccess));
    writerInitThread = new std::thread(pirateInitWriters, std::ref(remotes), std::ref(options), std::ref(writerSuccess));
    readerInitThread->join();
    writerInitThread->join();
    delete readerInitThread;
    delete writerInitThread;

    if (readerSuccess || writerSuccess) {
        return -1;
    }

    // block SIGINT for all threads except for the signalThread
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    OrientationOutput *orientationOutput;

    std::vector<std::shared_ptr<OrientationInput>> orientationInputs;
    std::shared_ptr<ColorTracking> colorTracking = nullptr;

    std::unique_ptr<OrientationOutput> delegate(OrientationOutputCreator::get(options));
    orientationOutput = new RemoteOrientationOutput(std::move(delegate), options, remotes);
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
        frameProcessors.push_back(colorTracking);
    }

    VideoSource *videoSource = VideoSourceCreator::create(options.mVideoInputType, frameProcessors, options);

    rv = orientationOutput->init();
    if (rv != 0)
    {
        return -1;
    }

    for (auto orientationInput : orientationInputs) {
        rv = orientationInput->init();
        if (rv != 0)
        {
            return -1;
        }
    }

    for (auto frameProcessor : frameProcessors) {
        rv = frameProcessor->init();
        if (rv != 0)
        {
            return -1;
        }
    }

    rv = videoSource->init();
    if (rv != 0)
    {
        videoSource->term();
        return -1;
    }

    signalThread = new std::thread(waitInterrupt, nullptr);

    while (!interrupted)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    signalThread->join();

    delete signalThread;
    delete videoSource;
    frameProcessors.clear();
    orientationInputs.clear();
    delete orientationOutput;

    pirateCloseRemotes(remotes);

    return 0;
}
