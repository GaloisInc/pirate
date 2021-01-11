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
#include <sys/ptrace.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "libpirate.h"

#include "cameracontrolinputcreator.hpp"
#include "cameracontroloutputcreator.hpp"
#include "remotecameracontroloutput.hpp"
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
            msg.str(std::string());
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
        msg.str(std::string());
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
        msg.str(std::string());
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
            msg.str(std::string());
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
    int rv = 0, readerSuccess, writerSuccess;
    bool tracing = false;
    Options options;
    RemoteDescriptors remotes;
    sigset_t set;
    struct sigaction newaction;
    std::thread *signalThread = nullptr, *readerInitThread, *writerInitThread;
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

    CameraControlOutput *cameraControlOutput = nullptr;

    std::vector<std::shared_ptr<CameraControlInput>> cameraControlInputs;
    std::shared_ptr<ColorTracking> colorTracking = nullptr;

    std::unique_ptr<CameraControlOutput> delegate(CameraControlOutputCreator::get(options));
    cameraControlOutput = new RemoteCameraControlOutput(std::move(delegate), options, remotes);
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

    if (options.mH264Encoder) {
        FrameProcessorCreator::add(H264Stream, frameProcessors, options, cameraControlCallbacks);
    }

    if (options.mImageTracking) {
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
    cameraControlInputs.clear();
    if (cameraControlOutput != nullptr) {
        delete cameraControlOutput;
    }

    pirateCloseRemotes(remotes);

    return rv;
}
