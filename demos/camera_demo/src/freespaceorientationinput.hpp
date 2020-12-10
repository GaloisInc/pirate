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

// Angular position is read from the Hillcrest Labs FSM9 AGM device

#include <thread>
#include <string>
#include <vector>
#include <freespace/freespace.h>
#include "orientationinput.hpp"

class FreespaceOrientationInput : public OrientationInput
{
public:
    FreespaceOrientationInput(
            CameraOrientationCallbacks angPosCallbacks,
            unsigned periodUs = DEFAULT_PERIOD_US);
    virtual ~FreespaceOrientationInput();

    virtual int init();
    virtual void term();

    static constexpr unsigned DEFAULT_PERIOD_US = 100000;
    static constexpr float GRAVITY_ACC = 9.81;
private:
    FreespaceDeviceId mDeviceId;
    unsigned mPeriodUs;

    static int setSensorPeriod(FreespaceDeviceId deviceId, unsigned periodUs);
    static int sensorEnable(FreespaceDeviceId deviceId, bool enable);

    static const std::vector<std::string> SENSOR_NAMES;
    static constexpr unsigned TIMEOUT_MS = 200;

    std::thread *mPollThread;
    bool mPoll;
    void pollThread();

    static void printVersionInfo();
    static int printDeviceInfo(FreespaceDeviceId deviceId);
    static int printSensorInfo(FreespaceDeviceId deviceId);

    // Simple weighted average FIR is used for smoothing
    static constexpr unsigned FIR_LEN = 8;   // must be power of 2
    static const float FIR_COEFFS[FIR_LEN];
    float mPrevPan[FIR_LEN], mPrevTilt[FIR_LEN];
    unsigned mPanIndex, mTiltIndex;
    float weightedFilter(float angularPosition, float previous[], unsigned &index);
    static inline unsigned nextFirIndex(unsigned index)
    {
        return (++index) & (FIR_LEN - 1);
    }
};
