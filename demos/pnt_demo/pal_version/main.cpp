// pnt_example.cpp : Defines the entry point for the console application.
//
#include "channel_fd.h"
#include "pnt_data.h"
#include "sensors.h"
#include "ownship.h"
#include "target.h"
#include "timer.h"
#include <mutex>
#include <string.h>

#include <pal/pal.h>

#define PIRATE_ENCLAVE_MAIN(e)  __attribute__((pirate_enclave_main(e)))

/* AUTOMATIC RESOURCES */

#define PIRATE_RESOURCE(name, enclave) __attribute__((pirate_resource(name, enclave),\
					       pirate_resource_param("required","",enclave)))
#define PIRATE_RESOURCE_TYPE(t)        __attribute__((pirate_resource_type(t)))
#define PIRATE_DOC(doc)                __attribute__((pirate_resource_param("doc", doc)))

#ifndef __GAPS__
#error "gaps compiler must be used"
#endif

#pragma pirate enclave declare(green)
#pragma pirate enclave capability(green, gps)

#pragma pirate enclave declare(orange)
#pragma pirate enclave capability(orange, rfsensor)

typedef std::string               string_resource       PIRATE_RESOURCE_TYPE("string");
typedef int64_t                   milliseconds_resource PIRATE_RESOURCE_TYPE("milliseconds");
typedef bool                      bool_resource         PIRATE_RESOURCE_TYPE("bool");

string_resource gpsToUAVPath
  PIRATE_RESOURCE("gps-to-uav", "orange")
  PIRATE_RESOURCE("gps-to-uav", "green")
  PIRATE_DOC("libpirate connection string from GPS to UAV");

string_resource uavToTargetPath
  PIRATE_RESOURCE("uav-to-target", "green")
  PIRATE_RESOURCE("uav-to-target", "orange")
  PIRATE_DOC("libpirate connection string from UAV to Target");

string_resource rfToTargetPath
  PIRATE_RESOURCE("rf-to-target", "green")
  PIRATE_RESOURCE("rf-to-target", "orange")
  PIRATE_DOC("libpirate connection string from RF to Target");

milliseconds_resource gpsDuration
  PIRATE_RESOURCE("duration", "green")
  PIRATE_DOC("milliseconds between simulation steps");

bool_resource fixedPeriod
  PIRATE_RESOURCE("fixed-period", "orange")
  PIRATE_DOC("Runs simulation in a deterministic mode");

int run_green(int argc, char** argv) PIRATE_ENCLAVE_MAIN("green")
{
  // Create channels
  auto gpsSend    = pirateSender<Position>(gpsToUAVPath);       // Green to orange
  auto uavToTargetRecv = pirateReceiver<Position>(uavToTargetPath);
  auto rfToTargetRecv  = pirateReceiver<Distance>(rfToTargetPath);

  Time start;

  // Create GPS
  GpsSensor gps(start, Position(.0, .0, .0), Velocity(50, 25, 12));
  // Create target
  Target tgt(10); // Updates at 10 Hz frequency

  // Create mutex for locking access to target.
  std::mutex tgtMutex;

  // Event to fire when position changes.
  gps.onPositionUpdate =
    [&tgtMutex, &tgt, gpsSend](const Position& p) {
      {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.onGpsPositionChange(p);
      }
      gpsSend(p);
    };

  std::thread uavToTargetThread =
    startReadMessages(uavToTargetRecv,
      [&tgt, &tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.setUAVLocation(p);
      });

  std::thread rfToTargetThread =
    startReadMessages(rfToTargetRecv,
      [&tgt, &tgtMutex](const Distance& d) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.setDistance(d);
      });


  // Run GPS every 10 milliseconds for the duration.
  onTimer(start, std::chrono::milliseconds(gpsDuration), std::chrono::milliseconds(10),
           [&gps](TimerMsec now){ gps.read(now); });

  // Close GPS
  gpsSend.close();
  // Wait for all target threads to terminate.
  rfToTargetThread.join();
  uavToTargetThread.join();
  return 0;
}

int run_orange(int argc, char** argv) PIRATE_ENCLAVE_MAIN("orange")
{
  // Create a function to get the current time that allows
  // fixed duration for deterministic execution.
  std::function<TimerMsec()> getTime
    = fixedPeriod
    ? fixedClock(std::chrono::milliseconds(10))
    : systemClock<std::chrono::milliseconds>;

  // Create channels (Note: Order must match corresponding run_green channel creation order)
  auto gpsToUAVRecv    = pirateReceiver<Position>(gpsToUAVPath);   // Green to orange
  auto uavToTargetSend = pirateSender<Position>(uavToTargetPath);  // Orange to green
  auto rfToTargetSend  = pirateSender<Distance>(rfToTargetPath);   // Orange to green

  // Create RF sensor
  Distance dtgt(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor rfs(getTime(), dtgt, vtgt);
  rfs.onDistanceUpdate = rfToTargetSend;

  // Create UAV and have it start listening.
  OwnShip uav(100); // updates at 100 Hz frequency
  uav.onUpdateTrack = uavToTargetSend;

  auto gpsRecvThread =
    startReadMessages(gpsToUAVRecv,
      [getTime, &rfs, &uav](const Position& p) {
        // Update RF sensor on GPS receive so that we are in sync.
        // Note. We could send RF and UAV data simultaneously to reduce
        // number of messages here.
        rfs.read(getTime());
        uav.onGpsPositionChange(p);
      });

  // Wait for GPS receive events to be closed.
  gpsRecvThread.join();

  rfToTargetSend.close();
  uavToTargetSend.close();

  return 0;
}
