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

int pirateValidateGapsOptions(Options &options) {
    if (options.mHasInput && options.mGapsRequestChannel.empty()) {
        std::cerr << "--gaps_req argument required for keyboard, freespace, or color tracking" << std::endl;
        return -1;
    }
    if (options.mHasOutput && options.mGapsRequestChannel.empty()) {
        std::cerr << "--gaps_req argument required for servo or print output types" << std::endl;
        return -1;
    }
    if (options.mImageSlidingWindow && options.mGapsResponseChannel.empty()) {
        std::cerr << "--gaps_rsp argument required for sliding window" << std::endl;
        return -1;
    }
    return 0;
}

void pirateInitReaders(RemoteDescriptors &remotes, const Options &options, int &success) {
    int rv;
    success = -1;
    // pirateInitReaders() and pirateInitWriters() both writing to std::cout
    std::stringstream msg;

    if (options.mHasOutput) {
        for (std::string channelDesc : options.mGapsRequestChannel) {
            rv = pirate_open_parse(channelDesc.c_str(), O_RDONLY);
            if (rv < 0) {
                perror("unable to open gaps request channel for reading");
                return;
            }
            remotes.mGapsRequestReadGds.push_back(rv);
            msg << "Opened " << channelDesc << " for reading." << std::endl;
            std::cout << msg.str();
            msg.clear();
        }
    }

    if (options.mHasInput && !options.mGapsResponseChannel.empty()) {
        std::string channelDesc = options.mGapsResponseChannel[0];
        rv = pirate_open_parse(channelDesc.c_str(), O_RDONLY);
        if (rv < 0) {
            perror("unable to open gaps response channel for reading");
            return;
        }
        remotes.mGapsResponseReadGd = rv;
        msg << "Opened " << channelDesc << " for reading." << std::endl;
        std::cout << msg.str();
        msg.clear();
    }

    success = 0;
}

void pirateInitWriters(RemoteDescriptors &remotes, Options &options, int &success) {
    int rv;
    success = -1;
    // pirateInitReaders() and pirateInitWriters() both writing to std::cout
    std::stringstream msg;

    if (options.mHasInput && !options.mGapsRequestChannel.empty()) {
        std::string channelDesc = options.mGapsRequestChannel[0];
        rv = pirate_open_parse(channelDesc.c_str(), O_WRONLY);
        if (rv < 0) {
            perror("unable to open gaps request channel for writing");
            return;
        }
        remotes.mGapsRequestWriteGd = rv;
        msg << "Opened " << channelDesc << " for writing." << std::endl;
        std::cout << msg.str();
        msg.clear();
    }

    if (options.mHasOutput) {
        for (std::string channelDesc : options.mGapsResponseChannel) {
            rv = pirate_open_parse(channelDesc.c_str(), O_WRONLY);
            if (rv < 0) {
                perror("unable to open gaps response channel for writing");
                return;
            }
            remotes.mGapsResponseWriteGds.push_back(rv);
            msg << "Opened " << channelDesc << " for writing." << std::endl;
            std::cout << msg.str();
            msg.clear();
        }
    }

    success = 0;
}

void pirateCloseRemotes(const RemoteDescriptors &remotes) {
    if (remotes.mGapsRequestWriteGd > 0) {
        pirate_close(remotes.mGapsRequestWriteGd);
    }
    if (remotes.mGapsResponseReadGd > 0) {
        pirate_close(remotes.mGapsResponseReadGd);
    }
    for (auto gd : remotes.mGapsResponseWriteGds) {
        if (gd > 0) {
            pirate_close(gd);
        }
    }
    for (auto gd : remotes.mGapsRequestReadGds) {
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
