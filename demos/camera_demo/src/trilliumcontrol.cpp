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

#include "trilliumcontrol.hpp"
#include "trilliumutilities.hpp"

#include "orion-sdk/OrionPublicPacket.hpp"
#include "orion-sdk/OrionPublicPacketShim.hpp"
#include "orion-sdk/Constants.hpp"
#include "orion-sdk/OrionComm.hpp"
#include "orion-sdk/Constants.hpp"

TrilliumControl::TrilliumControl(const Options& options) :
    BaseOrientationOutput(options),
    mTrilliumIpAddress(options.mTrilliumIpAddress),
    mSockFd(-1),
    mVerbose(options.mVerbose),
    mReceiveThread(nullptr),
    mReceive(false)
{
}

TrilliumControl::~TrilliumControl()
{
    term();
}

int TrilliumControl::init()
{
    int rv = -1;
    PanTilt initialPos(0.0, 0.0);

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
    cmd.Mode = ORION_MODE_POSITION;
    cmd.Stabilized = 0;
    cmd.ImpulseTime = 0;

    encodeOrionCmdPacket(&pkt, &cmd);
    return trilliumPktSend(mSockFd, pkt) == 0;
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
            processGeolocateTelemetry(pkt);
        break;

        default:
            std::cout << "Orion Packet:\n\tID  = "
                      << std::setw(3) << (unsigned)pkt.ID
                      << "\n\tLEN = " << (unsigned)pkt.Length 
                      << std::endl;
            break;
    }
}

void TrilliumControl::processSoftwareDiagnostics(OrionPkt_t& pkt)
{
    std::string board = "NONE";
    OrionSoftwareDiagnostics_t d;
    int rv = decodeOrionSoftwareDiagnosticsPacketStructure(&pkt, &d);
    if (rv != 1)
    {
        return;
    }

    switch (d.sourceBoard)
    {
    case BOARD_CLEVIS:    board = "CLEVIS";    break;
    case BOARD_CROWN:     board = "CROWN";     break;
    case BOARD_PAYLOAD:   board = "PAYLOAD";   break;
    case BOARD_LENSCTRL:  board = "LENSCTRL";  break;
    case BOARD_MISSCOMP:  board = "MISSCOMP";  break;

    case BOARD_NONE:
    default:
        break;
    }

    std::cout << "SOFTWARE DIAGNOSTICS:\n\t"
              << "Board             " << board << "\n\t"
              << "Cores             " << (unsigned)d.numCores << "\n\t"
              << "LOADING:";
    for (unsigned i = 0; i < d.numCores; i++)
    {
        std::cout << "\n\t\tCORE " << i + i << "\n\t\t\t"
                  << "CPU Load     " << d.CoreLoading[i].cpuLoad << " percent\n\t\t\t"
                  << "# threads    " << (unsigned)d.CoreLoading[i].numThreads
                  << std::endl;
    }
}

void TrilliumControl::processGeolocateTelemetry(OrionPkt_t& pkt)
{
    GeolocateTelemetryCore_t d;
    int rv = decodeGeolocateTelemetryCorePacketStructure(&pkt, &d);
    if (rv != 1)
    {
        return;
    }

    std::cout << "GEOLOCATE TELEMETRY:\n\t"
              << "System Time       " << d.systemTime << " ms\n\t"
              << "Position:\n\t\t"
              << std::setprecision(6) << std::fixed
              << "Latitude     " << rad2degf(d.posLat) << " deg\n\t\t"
              << "Longitude    " << rad2degf(d.posLon) << " deg\n\t\t"
              << "Altitude     " << d.posAlt << " meters\n\t"
              << "Camera:\n\t\t"
              << "Pan          " << rad2degf(d.pan) << " deg\n\t\t"
              << "Tilt         " << rad2degf(d.tilt) << " deg\n\t\t"
              << "HFOV         " << rad2degf(d.hfov) << " deg\n\t\t"
              << "VFOV         " << rad2degf(d.vfov) << " deg\n\t\t"
              << "Width        " << d.pixelWidth << " pixels\n\t\t"
              << "Height       " << d.pixelHeight << " pixels\n\t\t"
              << std::endl;

}
