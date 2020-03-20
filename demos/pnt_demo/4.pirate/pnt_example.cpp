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
  auto gpsToUAVSend    = pirateSender<std::pair<TimerMsec, Position>>(gpsToUAVPath, 0);       // Green to orange
  auto uavToTargetRecv = pirateReceiver<Position>(uavToTargetPath, 1);  // Orange to green
  auto rfToTargetRecv  = pirateReceiver<Distance>(rfToTargetPath, 2);   // Orange to green

  // Function to call when gps changes to update target.

  Time start;

  // CreateGPS
  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);
  GpsSensor gps(start, p, v);

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
  std::function<void(Position)> onGPSChange =
    [&tgt, &tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(tgtMutex);
        tgt.onGpsPositionChange(p);
      };

  // Run every 10 gps milliseconds.
  onTimer(start, duration,std::chrono::milliseconds(10), 
           [&gps, onGPSChange, gpsToUAVSend](TimerMsec now){ 
               gps.read(now); 
               onGPSChange(gps.getPosition());
               gpsToUAVSend(std::make_pair(now, gps.getPosition()));
             });

  // Close GPS  
  gpsToUAVSend.close();
  
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
  std::chrono::milliseconds duration;
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

  // Create channels (Note: Order must match corresponding run_green channel creation order)
  auto gpsToUAVRecv    = pirateReceiver<std::pair<TimerMsec, Position>>(gpsToUAVPath, 1);    // Green to orange
  auto uavToTargetSend = pirateSender<Position>(uavToTargetPath, 2);  // Orange to green
  auto rfToTargetSend  = pirateSender<Distance>(rfToTargetPath, 3);   // Orange to green

  Time start;

  // Create RF sensor
  Distance dtgt(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor rfs(start, rfToTargetSend, dtgt, vtgt);

  // Create UAV and have it start listening.
  OwnShip uav(uavToTargetSend, 100); // updates at 100 Hz frequency

  auto gpsToUAVThread =
    asyncReadMessages<std::pair<TimerMsec, Position>>(gpsToUAVRecv,
      [&rfs, &uav](const std::pair<TimerMsec, Position>& p) { 
          rfs.read(p.first);
          uav.onGpsPositionChange(p.second); 
        });

  // Run RF sensor
  //onTimer(start, duration, std::chrono::milliseconds(10), 
  //        [&rfs](TimerMsec now) { rfs.read(now); });

  // Wait for UAV to stop receiving messages and close its channel.    
  gpsToUAVThread.join();
  rfToTargetSend.close();
  uavToTargetSend.close();

  return 0;
}
