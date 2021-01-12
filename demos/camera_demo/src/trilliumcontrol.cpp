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

#include <thread>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>

#include "trilliumcontrol.hpp"
#include "trilliumutilities.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"
#include "orion-sdk/Constants.hpp"

static std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

static void tokenize(std::string const &str, const char delim, 
                        std::vector<std::string> &out)
{
    size_t start;
    size_t end = 0;
 
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        out.push_back(str.substr(start, end - start));
    }
}

TrilliumControl::TrilliumControl(const Options& options) :
    BaseCameraControlOutput(options),
    mTrilliumIpAddress(options.mTrilliumIpAddress),
    mTrilliumConfig(options.mTrilliumConfig),
    mSockFd(-1),
    mVerbose(options.mVerbose),
    mReceiveThread(nullptr),
    mReceive(false),
    mFlip(options.mImageFlip)
{
    std::memset(&mState, 0, sizeof(mState));
}

TrilliumControl::~TrilliumControl()
{
    term();
}

int TrilliumControl::init()
{
    int rv = -1;
    PanTilt initialPos(0.0, 0.0);

    if (mTrilliumIpAddress.empty())
    {
        std::cerr << "trillium address must be specified on the command-line" << std::endl;
        return -1;
    }

    rv = trilliumConnectUDPSocket(mTrilliumIpAddress, mSockFd);
    if (rv != 0)
    {
        return rv;
    }

    mReceive = true;
    mReceiveThread = new std::thread(&TrilliumControl::reveiveThread, this);

    // Trillium needs to be told twice
    for (int i = 0 ; i < 2; i++)
    {
        if (!applyAngularPosition(initialPos))
        {
            return -1;
        }
        usleep(1000);
    }

    rv = trilliumSensorConfig();
    if (rv != 0)
    {
        std::cerr << "Failed to configure Trillium Aptina sensor" << std::endl;
        return -1;
    }

    return 0;
}

void TrilliumControl::term()
{
    // Stop the receive
    if (mReceiveThread != nullptr)
    {
        mReceive = false;
        mReceiveThread->join();
        delete mReceiveThread;
        mReceiveThread = nullptr;
    }

    if (mSockFd >= 0)
    {
        close(mSockFd);
    }
}


bool TrilliumControl::applyAngularPosition(PanTilt angularPosition)
{
    OrionCmd_t cmd;
    OrionPkt_t pkt;

    if (mSockFd < 0)
    {
        return false;
    }

    std::memset(&cmd, 0, sizeof(cmd));
    cmd.Target[0] = deg2radf(angularPosition.pan);
    cmd.Target[1] = deg2radf(angularPosition.tilt);
    if (mFlip)
    {
        cmd.Target[0] = -cmd.Target[0];
        cmd.Target[1] = -cmd.Target[1];
    }
    cmd.Mode = ORION_MODE_POSITION;
    cmd.Stabilized = 0;
    cmd.ImpulseTime = 0;

    encodeOrionCmdPacket(&pkt, &cmd);
    return trilliumPktSend(mSockFd, pkt) == 0;
}

void TrilliumControl::updateZoom(CameraZoom zoom)
{
    int rv = -1;
    OrionPkt_t pkt;

    switch(zoom)
    {
        case Increment:
            mState.mZoom += mZoomIncrement;
            break;

        case Decrement:
            mState.mZoom -= mZoomIncrement;
            break;

        case Reset:
            mState.mZoom = mZoomDefault;
            break;

        default:
            return;
    }

    if (mState.mZoom < mZoomMin)
    {
        mState.mZoom = mZoomMin;
    }
    else if (mState.mZoom > mZoomMax)
    {
        mState.mZoom = mZoomMax;
    }

    encodeOrionCameraStatePacket(&pkt, mState.mZoom, -1, 0);

    rv = trilliumPktSend(mSockFd, pkt);
    if (rv != 0)
    {
        std::perror("Failed to send Trillium zoom command");
    }
}

int TrilliumControl::trilliumSensorConfig()
{
    int rv = -1;
    OrionPkt_t pkt;

    // Default configuration values
    mState.mAptina.Index       = 0;
    mState.mAptina.MinExposure = 0.1;
    mState.mAptina.MaxExposure = 8.0;
    mState.mAptina.MinGain     = 1;
    mState.mAptina.MaxGain     = 32;
    mState.mAptina.Brightness  = -8;
    mState.mAptina.Contrast    = -0.4;
    mState.mAptina.Saturation  = 0.3;
    mState.mAptina.Sharpness   = 2.0;
    mState.mAptina.DebugEnable = 0;
    mState.mAptina.Hue         = 0;

    if (!mTrilliumConfig.empty())
    {
        rv = trilliumConfigParse(mTrilliumConfig, mState.mAptina);
        if (rv)
        {
            return rv;
        }
    }

    encodeOrionAptinaSettingsPacketStructure(&pkt, &mState.mAptina);
    return trilliumPktSend(mSockFd, pkt);
}

int TrilliumControl::trilliumConfigParse(std::string file, OrionAptinaSettings_t &cfg)
{
    std::ifstream infile(file);
    std::string line;
    if (!infile.is_open())
    {
        std::perror("Failed to open Trillium configuration file");
        return -1;
    }

    while (std::getline(infile, line))
    {
        std::vector<std::string> cfgs;

        line = trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        tokenize(line, '=', cfgs);

        if (cfgs.size() != 2)
        {
            std::cout << "Invalid configuration: " << line << std::endl;
            return -1;
        }

        std::string opt = trim(cfgs[0]);
        std::istringstream val = std::istringstream(trim(cfgs[1]));

        if (opt.compare("MinExposure") == 0)       { val >> cfg.MinExposure; }
        else if (opt.compare("MaxExposure") == 0)  { val >> cfg.MaxExposure; }
        else if (opt.compare("MinGain") == 0)      { val >> cfg.MinGain;     }
        else if (opt.compare("MaxGain") == 0)      { val >> cfg.MaxGain;     }
        else if (opt.compare("Brightness") == 0)   { cfg.Brightness = std::stoi(val.str());  }
        else if (opt.compare("Contrast") == 0)     { val >> cfg.Contrast;    }
        else if (opt.compare("Saturation") == 0)   { val >> cfg.Saturation;  }
        else if (opt.compare("Sharpness") == 0)    { val >> cfg.Sharpness;   }
        else if (opt.compare("Hue") == 0)          { cfg.Hue = std::stoi(val.str());         }
        else
        {
            std::cout << "Invalid configuration option: " << opt << std::endl;
            return -1;
        }
    }

    return 0;
}

void TrilliumControl::reveiveThread()
{
    int rv = -1;
    OrionPkt_t pkt;

    while(mReceive)
    {
        rv = trilliumPktRecv(mSockFd, pkt);
        if ((rv == 0) && mVerbose)
        {
            processTrilliumPacket(pkt);
        }
    }
}

void TrilliumControl::processTrilliumPacket(OrionPkt_t& pkt)
{
    switch (pkt.ID)
    {
        case ORION_PKT_SOFTWARE_DIAGNOSTICS:
            processSoftwareDiagnostics(pkt);
            break;
        
        case ORION_PKT_GEOLOCATE_TELEMETRY:
            decodeGeolocateTelemetryCorePacketStructure(&pkt, &mState.mGeo);
            break;

        case ORION_PKT_DIAGNOSTICS:
            decodeOrionDiagnosticsPacketStructure(&pkt, &mState.mDiag);
            break;

        case ORION_PKT_CAMERA_STATE:
            processCameraState(pkt);
            break;

        case ORION_PKT_APTINA_SETTINGS:
            decodeOrionAptinaSettingsPacketStructure(&pkt, &mState.mAptina);
            break;

        case ORION_PKT_GPS_DATA:
            decodeGpsDataPacketStructure(&pkt, &mState.mGps);
            break;

        default:
            break;
    }

    mState.mLastPacket.mID = pkt.ID;
    mState.mLastPacket.mLength = pkt.Length;
    mState.mPacketCount++;
    printCameraStatus();
}

void TrilliumControl::processSoftwareDiagnostics(OrionPkt_t& pkt)
{
    OrionSoftwareDiagnostics_t d;
    int rv = decodeOrionSoftwareDiagnosticsPacketStructure(&pkt, &d);
    if (rv != 1 || d.sourceBoard >= BOARD_COUNT)
    {
        return;
    }

    mState.mSoftDiad[d.sourceBoard] = d;
}

void TrilliumControl::processCameraState(OrionPkt_t& pkt)
{
    float zoom = 0.0;
    float focus = 0.0;
    uint8_t index = ~0;

    int rv = decodeOrionCameraStatePacket(&pkt, &zoom, &focus, &index);
    if ((rv != 1) || (index != 0))
    {
        return;
    }

    mState.mZoom = zoom;
    mState.mFocus = focus;
}

void TrilliumControl::printCameraStatus()
{
    float lat = rad2degf(mState.mGeo.posLat);
    float lon = rad2degf(mState.mGeo.posLon);

    if ((fabsf(lat) < 0.001) && (fabsf(lon) < 0.001))
    {
        lat = 0.0;
        lon = 0.0;
    }

    const float alt = mState.mGeo.posAlt;
    const uint32_t time = mState.mGeo.systemTime;
    const float pan = rad2degf(mState.mGeo.pan);
    const float tilt = rad2degf(mState.mGeo.tilt);
    const float hfov = rad2degf(mState.mGeo.hfov);
    const float vfov = rad2degf(mState.mGeo.vfov);
    const uint32_t width = mState.mGeo.pixelWidth;
    const uint32_t height = mState.mGeo.pixelHeight;
    const uint32_t sats = mState.mGps.TrackedSats;

    const float minExposure = mState.mAptina.MinExposure;
    const float maxExposure = mState.mAptina.MaxExposure;
    const float minGain = mState.mAptina.MinGain;
    const float maxGain = mState.mAptina.MaxGain;
    const int brightness = mState.mAptina.Brightness;
    const float contrast = mState.mAptina.Contrast;
    const float saturation = mState.mAptina.Saturation;
    const float sharpness = mState.mAptina.Sharpness;
    const int hue = mState.mAptina.Hue;

    const float volt24 = mState.mDiag.Voltage24;
    const float volt12 = mState.mDiag.Voltage12;
    const float volt3v3 = mState.mDiag.Voltage3v3;
    const float curr24ma = mState.mDiag.Current24 * 1000.0;
    const float curr12ma = mState.mDiag.Current12 * 1000.0;
    const float curr3v3ma = mState.mDiag.Current3v3 * 1000.0;

    const unsigned clvsCores = mState.mSoftDiad[BOARD_CLEVIS].numCores;
    const unsigned clvsCpu1 = round(mState.mSoftDiad[BOARD_CLEVIS].CoreLoading[0].cpuLoad);
    const unsigned clvsThreads1 = mState.mSoftDiad[BOARD_CLEVIS].CoreLoading[0].numThreads;
    const unsigned clvsCpu2 = clvsCores == 2 ? round(mState.mSoftDiad[BOARD_CLEVIS].CoreLoading[1].cpuLoad) : 0;
    const unsigned clvsThreads2 = clvsCores == 2 ? mState.mSoftDiad[BOARD_CLEVIS].CoreLoading[1].numThreads : 0;

    const unsigned crownCores = mState.mSoftDiad[BOARD_CROWN].numCores;
    const unsigned crownCpu1 = round(mState.mSoftDiad[BOARD_CROWN].CoreLoading[0].cpuLoad);
    const unsigned crownThreads1 = mState.mSoftDiad[BOARD_CROWN].CoreLoading[0].numThreads;
    const unsigned crownCpu2 = crownCores == 2 ? round(mState.mSoftDiad[BOARD_CROWN].CoreLoading[1].cpuLoad) : 0;
    const unsigned crownThreads2 = crownCores == 2 ? mState.mSoftDiad[BOARD_CROWN].CoreLoading[1].numThreads : 0;

    const unsigned payloadCores = mState.mSoftDiad[BOARD_PAYLOAD].numCores;
    const unsigned payloadCpu1 = round(mState.mSoftDiad[BOARD_PAYLOAD].CoreLoading[0].cpuLoad);
    const unsigned payloadThreads1 = mState.mSoftDiad[BOARD_PAYLOAD].CoreLoading[0].numThreads;
    const unsigned payloadCpu2 = payloadCores == 2 ? round(mState.mSoftDiad[BOARD_PAYLOAD].CoreLoading[1].cpuLoad) : 0;
    const unsigned payloadThreads2 = payloadCores == 2 ? mState.mSoftDiad[BOARD_PAYLOAD].CoreLoading[1].numThreads : 0;

    const unsigned lensCtrlCores = mState.mSoftDiad[BOARD_LENSCTRL].numCores;
    const unsigned lensCtrlCpu1 = round(mState.mSoftDiad[BOARD_LENSCTRL].CoreLoading[0].cpuLoad);
    const unsigned lensCtrlThreads1 = mState.mSoftDiad[BOARD_LENSCTRL].CoreLoading[0].numThreads;
    const unsigned lensCtrlCpu2 = lensCtrlCores == 2 ? round(mState.mSoftDiad[BOARD_LENSCTRL].CoreLoading[1].cpuLoad) : 0;
    const unsigned lensCtrlThreads2 = lensCtrlCores == 2 ? mState.mSoftDiad[BOARD_LENSCTRL].CoreLoading[1].numThreads : 0;

    const unsigned missionCores = mState.mSoftDiad[BOARD_MISSCOMP].numCores;
    const unsigned missionCpu1 = round(mState.mSoftDiad[BOARD_MISSCOMP].CoreLoading[0].cpuLoad);
    const unsigned missionThreads1 = mState.mSoftDiad[BOARD_MISSCOMP].CoreLoading[0].numThreads;
    const unsigned missionCpu2 = missionCores == 2 ? round(mState.mSoftDiad[BOARD_MISSCOMP].CoreLoading[1].cpuLoad) : 0;
    const unsigned missionThreads2 = missionCores == 2 ? mState.mSoftDiad[BOARD_MISSCOMP].CoreLoading[1].numThreads : 0;

    const float crownT = mState.mDiag.CrownTemp;
    const float gyroT = mState.mDiag.GyroTemp;
    const float payloadT = mState.mDiag.PayloadTemp;

    std::stringstream s;

    s << std::string(100, '\n');
    s << "+" << std::string(78, '-') << "+\n";

    s << "| GEO:" << std::string(9, ' ')
      << "    Lat = " << std::setprecision(8) << std::setw(10) << lat << " deg "
      << "    Lon = "  << std::setprecision(8) << std::setw(10) << lon << " deg "
      << std::string(14, ' ') <<  "|\n|"
      << std::string(14, ' ')
      << "    Alt = " << std::setprecision(8) << std::setw(10) << alt << " m   "
      << "   Time = " << std::setw(10) << time
      << std::string(19, ' ') <<  "|\n|"
      << std::string(11, ' ')
      << "Satellites = " << std::setw(10) << sats
      << std::string(44, ' ') <<  "|\n";

    s << "+" << std::string(78, '-') << "+\n";

    s << "| CAMERA:" << std::string(6, ' ')
      << "    Pan = " << std::setprecision(8) << std::setw(10) << pan << " deg "
      << "   Tilt = " << std::setprecision(8) << std::setw(10) << tilt << " deg "
      << std::string(14, ' ') <<  "|\n";
    s << "|" << std::string(14, ' ')
      << "   HFOV = " << std::setprecision(8) << std::setw(10) << hfov << " deg "
      << "   VFOV = " << std::setprecision(8) << std::setw(10) << vfov << " deg "
      << std::string(14, ' ') <<  "|\n";
    s << "|" << std::string(14, ' ')
      << "   Zoom = " << std::setprecision(8) << std::setw(10) << mState.mZoom
      << std::string(5, ' ')
      << "  Focus = " << std::setprecision(8) << std::setw(10) << mState.mFocus
      << std::string(19, ' ') <<  "|\n";

    s << "|" << std::string(11, ' ')
      << "Resolution = " << std::setw(4) << width << " x " 
      << std::left << std::setw(4) << height
      << std::string(43, ' ') <<  "|\n";

    s << "+" << std::string(78, '-') << "+\n";
    s << "|      SENSOR:" 
      << "  Brightness " << std::left << std::setw(10) << brightness
      << "  Contrast   " <<  std::setprecision(8) << std::setw(10) << contrast
      << std::string(18, ' ') <<  "|\n";
    s << "|" << std::string(13, ' ')
      << "  Saturation " << std::setprecision(8) << std::setw(10) << saturation
      << "  Sharpness  " <<  std::setprecision(8) << std::setw(10) << sharpness
      << std::string(19, ' ') <<  "|\n";
    s << "|" << std::string(13, ' ')
      << "  Hue        " << std::left << std::setw(10) << hue
      << std::string(42, ' ') <<  "|\n";
    s << "|"
      << std::string(35, ' ') << "MIN" 
      << std::string(13, ' ') << "MAX"
      << std::string(24, ' ') <<  "|\n";
    s << "|" << std::string(13, ' ')
      << "  Exposure" << std::string(7, ' ')
      << std::setprecision(8) << std::setw(10) << minExposure << " ms    " 
      << std::setprecision(8) << std::setw(10) << maxExposure << " ms"
      << std::string(17, ' ') <<  "|\n";
    s << "|" << std::string(13, ' ')
      << "      Gain" << std::string(7, ' ')
      << std::setprecision(8) << std::setw(10) << minGain <<  std::string(8, ' ')
      << std::setprecision(8) << std::setw(10) << maxGain
      << std::string(20, ' ') <<  "|\n";

    s << "+" << std::string(78, '-') << "+\n";
    s << "|       POWER:                24V        12V        3.3V" 
      << std::string(23, ' ') <<  "|\n";
    s << "|              Voltage  (V)   "
      << std::setprecision(5) << std::setw(7) << volt24 << "    " 
      << std::setprecision(5) << std::setw(7) << volt12 << "    "
      << std::setprecision(5) << std::setw(7) << volt3v3
      << std::string(20, ' ') <<  "|\n";
    s << "|              Current (mA)   " 
      << std::setprecision(5) << std::setw(7) << curr24ma << "    " 
      << std::setprecision(5) << std::setw(7) << curr12ma << "    "
      << std::setprecision(5) << std::setw(7) << curr3v3ma
      << std::string(20, ' ') <<  "|\n";

    s << "+" << std::string(78, '-') << "+\n";

    s << "|    CPU LOAD: Cores     1:   CPU       Threads     2:   CPU       Threads     |\n";
    s << "|      CLEVIS      "
      << std::setw(2) << clvsCores << "         "
      << std::setw(3) << clvsCpu1 << "%      " << std::setw(3) << clvsThreads1
      << std::string(14, ' ')
      << std::setw(3) << clvsCpu2 << "%      " << std::setw(3) << clvsThreads2
      << std::string(9, ' ') << "|\n";
    s << "|       CROWN      "
      << std::setw(2) << crownCores << "         "
      << std::setw(3) << crownCpu1 << "%      " << std::setw(3) << crownThreads1
      << std::string(14, ' ')
      << std::setw(3) << crownCpu2 << "%      " << std::setw(3) << crownThreads2
      << std::string(9, ' ') << "|\n";
    s << "|     PAYLOAD      "
      << std::setw(2) << payloadCores << "         "
      << std::setw(3) << payloadCpu1 << "%      " << std::setw(3) << payloadThreads1
      << std::string(14, ' ')
      << std::setw(3) << payloadCpu2 << "%      " << std::setw(3) << payloadThreads2
      << std::string(9, ' ') << "|\n";
    s << "|   LENS CTRL      "
      << std::setw(2) << lensCtrlCores << "         "
      << std::setw(3) << lensCtrlCpu1 << "%      " << std::setw(3) << lensCtrlThreads1
      << std::string(14, ' ')
      << std::setw(3) << lensCtrlCpu2 << "%      " << std::setw(3) << lensCtrlThreads2
      << std::string(9, ' ') << "|\n";

    s << "|     MISSION      "
      << std::setw(2) << missionCores << "         "
      << std::setw(3) << missionCpu1 << "%      " << std::setw(3) << missionThreads1
      << std::string(14, ' ')
      << std::setw(3) << missionCpu2 << "%      " << std::setw(3) << missionThreads2
      << std::string(9, ' ') << "|\n";

    s << "+" << std::string(78, '-') << "+\n";

    s << "| TEMPERATURE:"
      << "  Crown = " << std::setprecision(3) << std::setw(3) << crownT   << " C   "
      << "   Gyro = " << std::setprecision(3) << std::setw(3) << gyroT    << " C   "
      << "Payload = " << std::setprecision(3) << std::setw(3) << payloadT << " C"
      << std::string(14, ' ') <<  "|\n";

    s << "+" << std::string(78, '-') << "+\n";
    std::cout << s.str() << std::endl;
}
