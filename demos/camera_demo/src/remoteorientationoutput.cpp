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

#include <iostream>
#include <vector>
#include <poll.h>

#include "libpirate.h"

#include "camerademo-serialization.hpp"
#include "remoteorientationoutput.hpp"

using namespace pirate;
using namespace CameraDemo;

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

RemoteOrientationOutput::RemoteOrientationOutput(
    std::unique_ptr<OrientationOutput> delegate,
    const Options& options, const RemoteDescriptors& remotes) :
        OrientationOutput(),
        mDelegate(std::move(delegate)),
        mMessageCounter(0),
        mHasOutput(options.mHasOutput),
        mGapsRequestWriteGd(remotes.mGapsRequestWriteGd),
        mGapsResponseReadGd(remotes.mGapsResponseReadGd),
        mGapsRequestReadGds(remotes.mGapsRequestReadGds),
        mGapsResponseWriteGds(remotes.mGapsResponseWriteGds),
        mClientLock(),
        mPollThread(nullptr),
        mPoll(false)
        {

        }

RemoteOrientationOutput::~RemoteOrientationOutput() {
    term();
}

int RemoteOrientationOutput::init() {
    int rv = 0;

    if (mHasOutput) {
        mPoll = true;
        mPollThread = new std::thread(&RemoteOrientationOutput::pollThread, this);
        rv = mDelegate->init();
    }
    return rv;
}

void RemoteOrientationOutput::term() {
    if (mPoll) {
        mPoll = false;
        if (mPollThread != nullptr) {
            mPollThread->join();
            delete mPollThread;
            mPollThread = nullptr;
        }
    }
}

int RemoteOrientationOutput::recvRequest(CameraDemo::OrientationOutputRequest& request, int &clientId) {
    int rv;
    struct pollfd fds[16];
    std::vector<char> readBuf(sizeof(struct OrientationOutputRequest_wire));
    int nfds = MIN(mGapsRequestReadGds.size(), 16);

    for (int i = 0; i < nfds; i++) {
        fds[i].fd = mGapsRequestReadGds[i];
        fds[i].events = POLLIN;
    }
    rv = poll(fds, nfds, 100);
    if (rv == 0) {
        return 0;
    } else if (rv < 0) {
        std::perror("remote orientation output receive request poll error");
        return -1;
    }
    int gd = -1;
    for (int i = 0; i < nfds; i++) {
        clientId = i;
        if (fds[i].revents & POLLIN) {
            gd = mGapsRequestReadGds[i];
            break;
        }
    }
    rv = pirate_read(gd, readBuf.data(), sizeof(struct OrientationOutputRequest_wire));
    if (rv < 0) {
        std::perror("remote orientation output receive request read error");
        return -1;
    } else if (rv == 0) {
        return 0;
    }
    if (rv != sizeof(struct OrientationOutputRequest_wire)) {
        std::cout << "orientation output request " << ((int) request.reqType)
            << " received " << rv << " out of " << sizeof(struct OrientationOutputRequest_wire)
            << " bytes" << std::endl;
        return -1;
    }
    request = Serialization<struct OrientationOutputRequest>::fromBuffer(readBuf);    
    return rv;
}

bool RemoteOrientationOutput::recvResponse(CameraDemo::OrientationOutputResponse& response) {
    int rv;

    std::vector<char> readBuf(sizeof(struct OrientationOutputResponse_wire));
    rv = pirate_read(mGapsResponseReadGd, readBuf.data(), sizeof(struct OrientationOutputResponse_wire));
    if (rv < 0) {
        std::perror("remote orientation output receive response read error");
        return false;
    }
    if (rv != sizeof(struct OrientationOutputResponse_wire)) {
        std::cout << "orientation output response received "
            << rv << " out of " << sizeof(struct OrientationOutputResponse_wire)
            << " bytes" << std::endl;
        return false;
    }
    response = Serialization<struct OrientationOutputResponse>::fromBuffer(readBuf);    
    return true;
}

bool RemoteOrientationOutput::sendRequest(const OrientationOutputRequest& request) {
    int rv;

    std::vector<char> writeBuf(sizeof(struct OrientationOutputRequest_wire));
    Serialization<struct OrientationOutputRequest>::toBuffer(request, writeBuf);
    rv = pirate_write(mGapsRequestWriteGd, writeBuf.data(), sizeof(struct OrientationOutputRequest_wire));
    if (rv < 0) {
        std::perror("remote orientation output send request write error");
        return false;
    }
    if (rv != sizeof(struct OrientationOutputRequest_wire)) {
        std::cout << "orientation output request " << ((int) request.reqType)
            << " sent " << rv << " out of " << sizeof(struct OrientationOutputRequest_wire)
            << " bytes" << std::endl;
        return false;
    }
    return true;
}

bool RemoteOrientationOutput::sendResponse(uint16_t id,
    const OrientationOutputResponse& response) {

    int rv, gd;

    if (id >= mGapsResponseWriteGds.size()) {
        std::cout << "invalid client id " << id << std::endl;
        return false;
    }
    gd = mGapsResponseWriteGds[id];
    std::vector<char> writeBuf(sizeof(struct OrientationOutputResponse_wire));
    Serialization<struct OrientationOutputResponse>::toBuffer(response, writeBuf);
    rv = pirate_write(gd, writeBuf.data(), sizeof(struct OrientationOutputResponse_wire));
    if (rv < 0) {
        std::perror("remote orientation output send response write error");
        return false;
    }
    if (rv != sizeof(struct OrientationOutputResponse_wire)) {
        std::cout << "orientation output response to client " << id
            << " sent " << rv << " out of " << sizeof(struct OrientationOutputResponse_wire)
            << " bytes" << std::endl;
        return false;
    }
    return true;
}

float RemoteOrientationOutput::getAngularPosition() {
    OrientationOutputRequest request;
    OrientationOutputResponse response;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = OrientationOutputReqType::OutputGet;
    request.messageId = mMessageCounter;
    request.angularPosition = std::numeric_limits<float>::quiet_NaN();
    bool success = sendRequest(request);
    if (success) {
        success = recvResponse(response);
    }
    mClientLock.unlock();
    if (success) {
        return response.angularPosition;
    } else {
        return std::numeric_limits<float>::quiet_NaN();
    }
}

void RemoteOrientationOutput::setAngularPosition(float angularPosition) {
    OrientationOutputRequest request;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = OrientationOutputReqType::OutputSet;
    request.messageId = mMessageCounter;
    request.angularPosition = angularPosition;
    sendRequest(request);
    mClientLock.unlock();
}

void RemoteOrientationOutput::updateAngularPosition(float positionUpdate) {
    OrientationOutputRequest request;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = OrientationOutputReqType::OutputUpdate;
    request.messageId = mMessageCounter;
    request.angularPosition = positionUpdate;
    sendRequest(request);
    mClientLock.unlock();
}

void RemoteOrientationOutput::pollThread() {
    OrientationOutputRequest request;
    OrientationOutputResponse response;

    while (mPoll) {
        int clientId = -1;
        int rv = recvRequest(request, clientId);
        if (rv < 0) {
            return;
        } else if (rv == 0) {
            continue;
        }
        switch (request.reqType) {
            case OrientationOutputReqType::OutputGet:
                response.angularPosition = mDelegate->getAngularPosition();
                sendResponse(clientId, response);
                break;
            case OrientationOutputReqType::OutputSet:
                mDelegate->setAngularPosition(request.angularPosition);
                break;
            case OrientationOutputReqType::OutputUpdate:
                mDelegate->updateAngularPosition(request.angularPosition);
                break;
        }
    }
}
