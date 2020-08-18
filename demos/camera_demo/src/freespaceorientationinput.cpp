#include <math.h>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <freespace/freespace_util.h>
#include "freespaceorientationinput.hpp"

const std::vector<std::string> FreespaceOrientationInput::SENSOR_NAMES = 
{
    "Accelerometer",
    "Gyroscope",
    "Magnetometer",
    "Ambient Light Sensor",
    "Pressure Sensor",
    "Proximity Sensor",
    "Sensor Fusion"
 };

const float FreespaceOrientationInput::FIR_COEFFS[FreespaceOrientationInput::FIR_LEN] = 
{
    1.0 / 16.0,
    1.0 / 16.0,
    1.0 / 16.0,
    1.0 / 16.0,
    2.0 / 16.0,
    2.0 / 16.0,
    4.0 / 16.0,
    4.0 / 16.0
};

FreespaceOrientationInput::FreespaceOrientationInput(
        AngularPosition<float>::UpdateCallback angPosUpdateCallback,
        float angPosMin, float angPosMax, unsigned periodUs) :
    OrientationInput(angPosUpdateCallback, angPosMin, angPosMax),
    mPeriodUs(periodUs),
    mPollThread(nullptr),
    mPoll(false),
    mFilterIndex(0)
{
    for (unsigned i = 0; i < FIR_LEN; i++)
    {
        mPrevAngPos[i] = 0.0;
    }
}

FreespaceOrientationInput::~FreespaceOrientationInput()
{
    term();
}

int FreespaceOrientationInput::init()
{
    // Display liblibfreespace version
    printVersionInfo();

    // Initialize the freespace library
    int rv = freespace_init();
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to initialize the Freespace library");
        return -1;
    }

    // Find a connected device
    int deviceCount = 0;
    rv = freespace_getDeviceList(&mDeviceId, 1, &deviceCount);
    if ((rv != FREESPACE_SUCCESS) || (deviceCount < 1))
    {
        std::perror("Failed to locate freespace device");
        return -1;
    }

    // Open the device
    rv = freespace_openDevice(mDeviceId);
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to open the freespace device");
        return -1;
    }

    // Print device info
    rv = printDeviceInfo(mDeviceId);
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to fetch the device info");
        return -1;
    }

    // Flush stale messages
    rv = freespace_flush(mDeviceId);
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to flush stale messages");
        return -1;
    }

    // Set the output period
    rv = setSensorPeriod(mDeviceId, mPeriodUs);
    if (rv != 0)
    {
        return -1;
    }

    // Print sensor info
    rv = printSensorInfo(mDeviceId);
    if (rv != 0)
    {
        return -1;
    }

    // Start the reader thread
    mPoll = true;
    mPollThread = new std::thread(&FreespaceOrientationInput::pollThread, this);

    return 0;
}

void FreespaceOrientationInput::term()
{
    // Stop the reader thread
    if (mPollThread != nullptr)
    {
        mPoll = false;
        mPollThread->join();
        delete mPollThread;
        mPollThread = nullptr;
    }

    // Close the freespace device
    freespace_closeDevice(mDeviceId);

    // Cleanup the freespace library
    freespace_exit();
}

int FreespaceOrientationInput::setSensorPeriod(FreespaceDeviceId deviceId,
        unsigned periodUs)
{
    for (unsigned i = 0; i < SENSOR_NAMES.size(); ++i)
    {   
        // Set the period
        struct freespace_message m;
        std::memset(&m, 0, sizeof(m));
        m.messageType = FREESPACE_MESSAGE_SENSORPERIODREQUEST;
        m.sensorPeriodRequest.commit = (i == (SENSOR_NAMES.size() - 1));
        m.sensorPeriodRequest.get  = 0;
        m.sensorPeriodRequest.sensor = i;
        m.sensorPeriodRequest.period = periodUs;

        int rv = freespace_sendMessage(deviceId, &m);
        if (rv != FREESPACE_SUCCESS)
        {
            std::perror("Failed to set the sensor period");
            return -1;
        }

        // Wait for period set response
        do
        {
            rv = freespace_readMessage(deviceId, &m, TIMEOUT_MS);
            if (rv != FREESPACE_SUCCESS)
            {
                std::perror("Failed to get the sensor period set response");
                return -1;
            }
        } while (m.messageType != FREESPACE_MESSAGE_SENSORPERIODRESPONSE);

        if (m.sensorPeriodResponse.sensor != i)
        {
            std::perror("Invalid sensor index in sensor period get response");
            return -1;
        }
    }

    // Wait for changes to take effect
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return 0;
}

int FreespaceOrientationInput::sensorEnable(FreespaceDeviceId deviceId,
        bool enable)
{
    struct freespace_message m;
    std::memset(&m, 0, sizeof(m));
    m.messageType = FREESPACE_MESSAGE_DATAMODECONTROLV2REQUEST;
    m.dataModeControlV2Request.packetSelect = 8;    // Motion engine output
    m.dataModeControlV2Request.mode = 4;            // Set full motion
    m.dataModeControlV2Request.formatSelect = 0;    // MEOut format 0
    m.dataModeControlV2Request.ff0 = enable;        // Pointer fileds
    m.dataModeControlV2Request.ff1 = enable;        // Linear acceleration fields

    int rv = freespace_sendMessage(deviceId, &m);
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to set data mode");
        return -1;
    }

    return 0;
}

void FreespaceOrientationInput::pollThread()
{
    struct freespace_message m;
    struct MultiAxisSensor acc;

    std::memset(&m, 0, sizeof(m));
    std::memset(&acc, 0, sizeof(acc));

    const float slope = (mAngularPositionMax - mAngularPositionMin) / (GRAVITY_ACC * 2.0);
    const float offset = slope * GRAVITY_ACC + mAngularPositionMin;
    float angularPosition = 0.0;

    // Enable the data flow
    int rv = sensorEnable(mDeviceId, true);
    if (rv != 0)
    {
        return;
    }

    while (mPoll)
    {
        rv = freespace_readMessage(mDeviceId, &m, TIMEOUT_MS);
        if (rv != FREESPACE_SUCCESS)
        {
            continue;
        }

        if (m.messageType != FREESPACE_MESSAGE_MOTIONENGINEOUTPUT)
        {
            continue;
        }

        rv = freespace_util_getAcceleration(&m.motionEngineOutput, &acc);
        if (rv != FREESPACE_SUCCESS)
        {
            std::perror("Failed to extract acceleration values");
            continue;
        }

        angularPosition = weightedFilter(slope * (-1.0) * acc.y + offset);
        setAngularPosition(rint(angularPosition));
    }

    // Disable data flow
    sensorEnable(mDeviceId, false);
}

void FreespaceOrientationInput::printVersionInfo()
{
    std::cout << "libfreespace verson\n\t" << freespace_version() << std::endl;
}

int FreespaceOrientationInput::printDeviceInfo(FreespaceDeviceId deviceId)
{   
    struct FreespaceDeviceInfo info;

    int rv = freespace_getDeviceInfo(deviceId, &info);
    if (rv != FREESPACE_SUCCESS)
    {
        std::perror("Failed to get the freespace device info");
        return -1;
    }

    std::ios_base::fmtflags flags = std::cout.flags();
    std::cout << "Freespace Device:"
              << "\n\tDevice        " << info.name
              << std::uppercase << std::hex << std::setw(4)
              << "\n\tVendor ID     0x" << info.vendor
              << "\n\tProduct ID    0x" << info.product
              << std::endl;
    std::cout.flags(flags);
    return 0;
}

int FreespaceOrientationInput::printSensorInfo(FreespaceDeviceId deviceId)
{
    std::cout << "Sensors:\n";
    for (unsigned sensor = 0; sensor < SENSOR_NAMES.size(); sensor++)
    {
        struct freespace_message m;
        std::memset(&m, 0, sizeof(m));

        // Request sensor period
        m.messageType = FREESPACE_MESSAGE_SENSORPERIODREQUEST;
        m.sensorPeriodRequest.get = 1;
        m.sensorPeriodRequest.sensor = sensor;
        int rv = freespace_sendMessage(deviceId, &m);
        if (rv != FREESPACE_SUCCESS)
        {
            std::perror("Failed to send sensor period request");
            return -1;
        }

        // Wait for sensor period
        do 
        {
            rv = freespace_readMessage(deviceId, &m, TIMEOUT_MS);
            if (rv != FREESPACE_SUCCESS)
            {
                std::perror("Failed to read sensor period");
                return -1;
            }
        } while (m.messageType != FREESPACE_MESSAGE_SENSORPERIODRESPONSE);

        std::cout << '\t' << sensor << ' '
                  << std::left << std::setw(20) << SENSOR_NAMES[sensor] << "  ";
        if (m.sensorPeriodResponse.period != 0)
        {
            std::cout << "period " << m.sensorPeriodResponse.period << " us";
        }
        else
        {
            std::cout << "disabled";
        }
        std::cout << '\n';
    }

    std::cout << std::endl;
    return 0;
}

float FreespaceOrientationInput::weightedFilter(float angularPosition)
{
    float ret = 0.0;
    mPrevAngPos[mFilterIndex] = angularPosition;
    mFilterIndex = nextFirIndex(mFilterIndex);
    
    for (unsigned i = 0; i < FIR_LEN; i++)
    {
        ret += FIR_COEFFS[i] * mPrevAngPos[(i + mFilterIndex) & (FIR_LEN - 1)];
    }

    return ret;
}
