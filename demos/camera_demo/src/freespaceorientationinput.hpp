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
            AngularPosition<float>::UpdateCallback angPosUpdateCallback,
            float angPosMin = -AngularPosition<float>::DEFAULT_ANG_POS_LIMIT,
            float angPosMax =  AngularPosition<float>::DEFAULT_ANG_POS_LIMIT,
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
    float mPrevAngPos[FIR_LEN];
    unsigned mFilterIndex;
    float weightedFilter(float angularPosition);
    static inline unsigned nextFirIndex(unsigned index)
    {
        return (++index) & (FIR_LEN - 1);
    }
};

