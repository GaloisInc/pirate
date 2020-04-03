// pnt_example.cpp : Defines the entry point for the console application.
//

#include "channel_fd.h"
#include "pnt_data.h"
#include "sensors.h"
#include "ownship.h"
#include "target.h"
#include "timer.h"
#include "resource_loader.h"

#include <mutex>
#include <string.h>

#pragma pirate enclave declare(green)
#pragma pirate enclave declare(orange)

#ifdef GAPS_DISABLE
#define PIRATE_ENCLAVE_MAIN(e)
#else
#ifndef __GAPS__
#error "gaps compiler must be used"
#endif
#pragma pirate enclave declare(green)
#pragma pirate enclave declare(orange)
#define PIRATE_ENCLAVE_MAIN(e)  __attribute__((pirate_enclave_main(e)))
#endif

/* AUTOMATIC RESOURCES */

#define RESOURCE(name, enclave) __attribute__((pirate_resource(name, enclave)))
#define RESOURCE_TYPE(t)        __attribute__((pirate_resource_type(t)))
#define DOC(doc)                __attribute__((pirate_resource_param("doc", doc)))

typedef std::string               string_resource       RESOURCE_TYPE("string");
typedef std::chrono::milliseconds milliseconds_resource RESOURCE_TYPE("milliseconds");
typedef bool                      bool_resource         RESOURCE_TYPE("bool");

string_resource gpsToUAVPath
  RESOURCE("gps-to-uav-path", "orange")
  RESOURCE("gps-to-uav-path", "green")
  DOC("libpirate connection string from GPS to UAV");

string_resource uavToTargetPath
  RESOURCE("uav-to-target-path", "green")
  RESOURCE("uav-to-target-path", "orange")
  DOC("libpirate connection string from UAV to Target");

string_resource rfToTargetPath
  RESOURCE("rf-to-target-path", "green")
  RESOURCE("rf-to-target-path", "orange")
  DOC("libpirate connection string from RF to Target");

milliseconds_resource timerDuration
  RESOURCE("timer-duration", "green")
  DOC("milliseconds between simulation steps");

bool_resource fixedPeriod
  RESOURCE("fixed-period", "orange")
  DOC("Runs simulation in a deterministic mode");

/* END OF RESOURCES */

int run_green(int argc, char** argv) PIRATE_ENCLAVE_MAIN("green")
{
  if (load_resources(argc, argv)) {
    std::cerr << "Failed to process commandline arguments" << std::endl;
    return EXIT_FAILURE;
  }

  // Create channels
//  piratePipe(gpsToTarget, 0);
//  auto gpsToTargetSend = gdSender<Position>(gpsToTarget, 0);            // Green to green
//  auto gpsToTargetRecv = gdReceiver<Position>(gpsToTarget, 0);          // Green to green
  auto gpsToUAVSend    = pirateSender<Position>(gpsToUAVPath);       // Green to orange
  auto uavToTargetRecv = pirateReceiver<Position>(uavToTargetPath);  // Orange to green
  auto rfToTargetRecv  = pirateReceiver<Distance>(rfToTargetPath);   // Orange to green

  std::function<void(Position)> onGPSChange;

  // Sender to call when gps changes to update target.
  Sender<Position> gpsSender(
        [&onGPSChange, gpsToUAVSend](const Position& p) {
          onGPSChange(p);
          gpsToUAVSend(p);
        },
        [&gpsToUAVSend]() {
          gpsToUAVSend.close();
        });

  Time start;

  // CreateGPS
  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);
  GpsSensor gps(start, gpsSender, p, v);

  // Create target and event handling threads.
  Target tgt(10); // updates at 10 Hz frequency
  std::mutex tgtMutex;
  auto uavToTargetThread = 
    asyncReadMessages<Position>(uavToTargetRecv,
      [&tgt, &tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.setUAVLocation(p);
      });
  std::thread rfToTargetThread(rfToTargetRecv,
    [&tgt, &tgtMutex](const Distance& d) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.setDistance(d);
      });
  // Call position change when received.
  onGPSChange =
    [&tgt, &tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.onGpsPositionChange(p);
      };

  // Run GPS every 10 milliseconds.
  onTimer(start, timerDuration,std::chrono::milliseconds(10), 
           [&gps](TimerMsec now){ gps.read(now); });

  // Close GPS  
  gpsSender.close();
  
  // Wait for all target threads to terminate.
  uavToTargetThread.join();
  rfToTargetThread.join();
  return 0;
}

int run_orange(int argc, char** argv) PIRATE_ENCLAVE_MAIN("orange")
{
  if (load_resources(argc, argv)) {
    std::cerr << "Failed to process commandline arguments" << std::endl;
    return EXIT_FAILURE;
  }

  // Create a function to get the current time that allows
  // fixed duration for deterministic execution.
  std::function<TimerMsec()> getTime;
  TimerMsec base;
  if (fixedPeriod) {
    getTime = [&base] () { 
      TimerMsec now = base;
      base = base + std::chrono::milliseconds(10);
      return now;
    };
  } else {
    getTime = [] () {
      return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
    };
  }

  // Create channels (Note: Order must match corresponding run_green channel creation order)
  auto gpsToUAVRecv    = pirateReceiver<Position>(gpsToUAVPath);    // Green to orange
  auto uavToTargetSend = pirateSender<Position>(uavToTargetPath);  // Orange to green
  auto rfToTargetSend  = pirateSender<Distance>(rfToTargetPath);   // Orange to green

  // Create RF sensor
  Distance dtgt(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor rfs(getTime(), rfToTargetSend, dtgt, vtgt);

  // Create UAV and have it start listening.
  OwnShip uav(uavToTargetSend, 100); // updates at 100 Hz frequency

  auto gpsToUAVThread =
    asyncReadMessages<Position>(gpsToUAVRecv,
      [getTime, &rfs, &uav](const Position& p) {
        // Update RF sensor on GPS receive so that we are in sync.
        // Note. We could send RF and UAV data simultaneously to reduce
        // number of messages here.
        rfs.read(getTime());
        uav.onGpsPositionChange(p); 
      });
  gpsToUAVThread.join();

  // Wait for UAV to stop receiving messages and close its channel.    
  rfToTargetSend.close();
  uavToTargetSend.close();

  return 0;
}
