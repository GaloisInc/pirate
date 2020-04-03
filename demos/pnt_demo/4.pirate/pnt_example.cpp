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

void showUsage(const char* arg0) {
  std::cerr
       << "Usage:\n"
       << "  "  << arg0 << " --gps-to-uav path --uav-to-target path --rf-to-target path\n"
       << std::endl
       << "  path should be a valid libpirate channel format string." << std::endl;
}

int run_green(int argc, char** argv) PIRATE_ENCLAVE_MAIN("green")
{
  // Parse command line arguments
  std::string gpsToTarget;
  std::string gpsToUAVPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
  std::chrono::milliseconds duration;
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--gps-to-target") == 0) {
      gpsToTarget = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--gps-to-uav") == 0) {
      gpsToUAVPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--uav-to-target") == 0) {
      uavToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--rf-to-target") == 0) {
      rfToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--duration") == 0) {
      duration = std::chrono::milliseconds(std::stoul(argv[i+1]));
      i+=2;
    } else if (strcmp(argv[i], "--help") == 0) {
      showUsage(argv[0]);
      exit(0);
    } else {
      std::cerr << "Unexpected argument " << argv[i] << std::endl;
      showUsage(argv[0]);
      exit(-1);
    }
  }

  // Create channels
//  piratePipe(gpsToTarget, 0);
//  auto gpsToTargetSend = gdSender<Position>(gpsToTarget, 0);            // Green to green
//  auto gpsToTargetRecv = gdReceiver<Position>(gpsToTarget, 0);          // Green to green
  auto gpsToUAVSend    = pirateSender<Position>(gpsToUAVPath);       // Green to orange
  auto uavToTargetRecv = pirateReceiver<Position>(uavToTargetPath);
  auto rfToTargetRecv  = pirateReceiver<Distance>(rfToTargetPath);

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
  std::thread rfToTargetThread = 
    asyncReadMessages<Distance>(rfToTargetRecv,
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
  onTimer(start, duration, std::chrono::milliseconds(10), 
           [&gps](TimerMsec now){ gps.read(now); });

  // Close GPS  
  gpsSender.close();
  uavToTargetRecv.close();
  rfToTargetRecv.close();
  
  // Wait for all target threads to terminate.
  uavToTargetThread.join();
  rfToTargetThread.join();
  return 0;
}

int run_orange(int argc, char** argv) PIRATE_ENCLAVE_MAIN("orange")
{
  // Parse command line arguments
  std::string gpsToUAVPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
  bool fixedPeriod = false;
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--gps-to-uav") == 0) {
      gpsToUAVPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--uav-to-target") == 0) {
      uavToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--rf-to-target") == 0) {
      rfToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--fixed") == 0) {
      fixedPeriod = true;
      i+=1;
    } else if (strcmp(argv[i], "--help") == 0) {
      showUsage(argv[0]);
      exit(0);
    } else {
      std::cerr << "Unexpected argument " << argv[i] << std::endl;
      showUsage(argv[0]);
      exit(-1);
    }
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
  gpsToUAVRecv.close();

  return 0;
}
