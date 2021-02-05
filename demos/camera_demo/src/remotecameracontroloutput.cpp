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
#include "remotecameracontroloutput.hpp"

using namespace pirate;
using namespace CameraDemo;

#ifndef MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

RemoteCameraControlOutput::RemoteCameraControlOutput(
    std::unique_ptr<CameraControlOutput> delegate,
    const Options& options, const RemoteDescriptors& remotes) :
        CameraControlOutput(),
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

RemoteCameraControlOutput::~RemoteCameraControlOutput() {
    term();
}

int RemoteCameraControlOutput::init() {
    int rv = 0;

    if (mHasOutput) {
        mPoll = true;
        mPollThread = new std::thread(&RemoteCameraControlOutput::pollThread, this);
        rv = mDelegate->init();
    }
    return rv;
}

void RemoteCameraControlOutput::term() {
    if (mPoll) {
        mPoll = false;
        if (mPollThread != nullptr) {
            mPollThread->join();
            delete mPollThread;
            mPollThread = nullptr;
        }
    }
}

int RemoteCameraControlOutput::recvRequest(CameraDemo::CameraControlOutputRequest& request, int &clientId) {
    int rv;
    struct pollfd fds[16];
    std::vector<char> readBuf(sizeof(struct CameraControlOutputRequest_wire));
    int nfds = MIN(mGapsRequestReadGds.size(), 16);

    for (int i = 0; i < nfds; i++) {
        fds[i].fd = mGapsRequestReadGds[i];
        fds[i].events = POLLIN;
    }
    rv = poll(fds, nfds, 100);
    if (rv == 0) {
        return 0;
    } else if (rv < 0) {
        std::perror("remote camera control output receive request poll error");
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
    rv = pirate_read(gd, readBuf.data(), sizeof(struct CameraControlOutputRequest_wire));
    if (rv < 0) {
        std::perror("remote camera control output receive request read error");
        return -1;
    } else if (rv == 0) {
        return 0;
    }
    if (rv != sizeof(struct CameraControlOutputRequest_wire)) {
        std::cout << "camera control output request " << ((int) request.reqType)
            << " received " << rv << " out of " << sizeof(struct CameraControlOutputRequest_wire)
            << " bytes" << std::endl;
        return -1;
    }
    request = Serialization<struct CameraControlOutputRequest>::fromBuffer(readBuf);    
    return rv;
}

bool RemoteCameraControlOutput::recvResponse(CameraDemo::CameraControlOutputResponse& response) {
    int rv;

    std::vector<char> readBuf(sizeof(struct CameraControlOutputResponse_wire));
    rv = pirate_read(mGapsResponseReadGd, readBuf.data(), sizeof(struct CameraControlOutputResponse_wire));
    if (rv < 0) {
        std::perror("remote camera control output receive response read error");
        return false;
    }
    if (rv != sizeof(struct CameraControlOutputResponse_wire)) {
        std::cout << "camera control output response received "
            << rv << " out of " << sizeof(struct CameraControlOutputResponse_wire)
            << " bytes" << std::endl;
        return false;
    }
    response = Serialization<struct CameraControlOutputResponse>::fromBuffer(readBuf);    
    return true;
}

bool RemoteCameraControlOutput::sendRequest(const CameraControlOutputRequest& request) {
    int rv;

    std::vector<char> writeBuf(sizeof(struct CameraControlOutputRequest_wire));
    Serialization<struct CameraControlOutputRequest>::toBuffer(request, writeBuf);
    rv = pirate_write(mGapsRequestWriteGd, writeBuf.data(), sizeof(struct CameraControlOutputRequest_wire));
    if (rv < 0) {
        std::perror("remote camera control output send request write error");
        return false;
    }
    if (rv != sizeof(struct CameraControlOutputRequest_wire)) {
        std::cout << "camera control output request " << ((int) request.reqType)
            << " sent " << rv << " out of " << sizeof(struct CameraControlOutputRequest_wire)
            << " bytes" << std::endl;
        return false;
    }
    return true;
}

bool RemoteCameraControlOutput::sendResponse(uint16_t id,
    const CameraControlOutputResponse& response) {

    int rv, gd;

    if (id >= mGapsResponseWriteGds.size()) {
        std::cout << "invalid client id " << id << std::endl;
        return false;
    }
    gd = mGapsResponseWriteGds[id];
    std::vector<char> writeBuf(sizeof(struct CameraControlOutputResponse_wire));
    Serialization<struct CameraControlOutputResponse>::toBuffer(response, writeBuf);
    rv = pirate_write(gd, writeBuf.data(), sizeof(struct CameraControlOutputResponse_wire));
    if (rv < 0) {
        std::perror("remote camera control output send response write error");
        return false;
    }
    if (rv != sizeof(struct CameraControlOutputResponse_wire)) {
        std::cout << "camera control output response to client " << id
            << " sent " << rv << " out of " << sizeof(struct CameraControlOutputResponse_wire)
            << " bytes" << std::endl;
        return false;
    }
    return true;
}

bool RemoteCameraControlOutput::equivalentPosition(PanTilt p1, PanTilt p2) {
    return mDelegate->equivalentPosition(p1, p2);
}

void RemoteCameraControlOutput::updateZoom(CameraZoom zoom) {
    (void) zoom; // TODO
}

PanTilt RemoteCameraControlOutput::getAngularPosition() {
    CameraControlOutputRequest request;
    CameraControlOutputResponse response;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = CameraControlOutputReqType::OutputGet;
    request.messageId = mMessageCounter;
    request.angularPositionPan = std::numeric_limits<float>::quiet_NaN();
    request.angularPositionTilt = std::numeric_limits<float>::quiet_NaN();
    bool success = sendRequest(request);
    if (success) {
        success = recvResponse(response);
    }
    mClientLock.unlock();
    if (success) {
        return PanTilt(response.angularPositionPan, response.angularPositionTilt);
    } else {
        return PanTilt(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    }
}

void RemoteCameraControlOutput::setAngularPosition(PanTilt angularPosition) {
    CameraControlOutputRequest request;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = CameraControlOutputReqType::OutputSet;
    request.messageId = mMessageCounter;
    request.angularPositionPan = angularPosition.pan;
    request.angularPositionTilt = angularPosition.tilt;
    sendRequest(request);
    mClientLock.unlock();
}

void RemoteCameraControlOutput::updateAngularPosition(PanTilt positionUpdate) {
    CameraControlOutputRequest request;

    mClientLock.lock();
    mMessageCounter++;
    request.reqType = CameraControlOutputReqType::OutputUpdate;
    request.messageId = mMessageCounter;
    request.angularPositionPan = positionUpdate.pan;
    request.angularPositionTilt = positionUpdate.tilt;
    sendRequest(request);
    mClientLock.unlock();
}

void RemoteCameraControlOutput::pollThread() {
    CameraControlOutputRequest request;
    CameraControlOutputResponse response;
    PanTilt angularPosition;

    while (mPoll) {
        int clientId = -1;
        int rv = recvRequest(request, clientId);
        if (rv < 0) {
            return;
        } else if (rv == 0) {
            continue;
        }
        switch (request.reqType) {
            case CameraControlOutputReqType::OutputGet:
                angularPosition = mDelegate->getAngularPosition();
                response.angularPositionPan = angularPosition.pan;
                response.angularPositionTilt = angularPosition.tilt;
                sendResponse(clientId, response);
                break;
            case CameraControlOutputReqType::OutputSet:
                angularPosition.pan = request.angularPositionPan;
                angularPosition.tilt = request.angularPositionTilt;
                mDelegate->setAngularPosition(angularPosition);
                break;
            case CameraControlOutputReqType::OutputUpdate:
                angularPosition.pan = request.angularPositionPan;
                angularPosition.tilt = request.angularPositionTilt;
                mDelegate->updateAngularPosition(angularPosition);
                break;
        }
    }
}
