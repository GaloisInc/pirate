// pnt_example.cpp : Defines the entry point for the console application.
//

#include "channel_fd.h"
#include "channel_pirate.h"
#include "pnt_data.h"
#include "sensors.h"
#include "ownship.h"
#include "target.h"
#include "timer.h"
#include <mutex>
#include <string.h>

void setupGps(Sender<Position> gpsSender)
{
  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);

  GpsSensor* gps = new GpsSensor(gpsSender, p, v);
  // Run every 10 gps milliseconds.
  startTimer(10, [gps](){ gps->read(msecs(10)); });
}

void setupTarget(Receiver<Position> uav, Receiver<Distance> rf, Receiver<Position> gps)
{
  Target* tgt = new Target(10); // updates at 10 Hz frequency
  auto tgtMutex = new std::mutex();

  asyncReadMessages<Position>(uav,
    [tgt, tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(*tgtMutex);
        tgt->setUAVLocation(p);
      });
  asyncReadMessages<Distance>(rf,
    [tgt, tgtMutex](const Distance& d) {
        std::lock_guard<std::mutex> g(*tgtMutex);
        tgt->setDistance(d);
      });
  asyncReadMessages<Position>(gps,
    [tgt, tgtMutex](const Position& p) {
      std::lock_guard<std::mutex> g(*tgtMutex);
      tgt->onGpsPositionChange(p);
    });
}

void setupRfSensor(Sender<Distance> rfSender)
{
  Distance d(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor* rfs  = new RfSensor(rfSender, d, vtgt);
  startTimer(sleep_msec, [rfs]() {
    print([](std::ostream& o) { o << "rf sensor event." << std::endl; });
    rfs->read(msecs(10));
  });
}

void setupUAV(Sender<Position> uavSender, Receiver<Position> gps)
{
  OwnShip* uav = new OwnShip(uavSender, 100); // updates at 100 Hz frequency
  asyncReadMessages<Position>(gps,
    [uav](const Position& p) {
        uav->onGpsPositionChange(p);
      });
}

#pragma pirate enclave declare(green)
#pragma pirate enclave declare(orange)

void showUsage(const char* arg0) {
  std::cerr
    << "Usage:\n"
    << "  "  << arg0 << " --gps-to-uav path --uav-to-target path --rf-to-target path\n"
    << std::endl
    << "  path should be a valid libpirate channel format string." << std::endl;
  exit(-1);
}

int run_green(int argc, char** argv) __attribute__((pirate_enclave_main("green")))
{
  std::string gpsToUAVPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
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
    } else if (strcmp(argv[i], "--help") == 0) {
      showUsage(argv[0]);
    } else {
      std::cerr << "Unexpected argument " << argv[i] << std::endl;
      showUsage(argv[0]);
    }
  }

  auto gpsToTargetChan = anonPipe<Position>(); // Green to green
  auto gpsToUAVSend    = pirateSender<Position>(  0, gpsToUAVPath, "gpsToUAV");    // Green to orange
  auto uavToTargetRecv = pirateReceiver<Position>(1, uavToTargetPath, "uavToTarget");  // Orange to green
  auto rfToTargetRecv  = pirateReceiver<Distance>(2, rfToTargetPath, "rfToTarget");   // Orange to green
  auto gpsToTargetSend = gpsToTargetChan.sender;

  // Setup sender for gps that broadcasts to two other channels.
  Sender<Position> gpsSender = [gpsToUAVSend, gpsToTargetSend](const Position& p) {
    gpsToUAVSend(p);
    gpsToTargetSend(p);
  };

  setupTarget(uavToTargetRecv, rfToTargetRecv, gpsToTargetChan.receiver);
  setupGps(gpsSender);
  usleep(5 * 1000 * 1000);
  return 0;
}

int run_orange(int argc, char** argv) __attribute__((pirate_enclave_main("orange")))
{
  std::string gpsToUAVPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
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
    } else if (strcmp(argv[i], "--help") == 0) {
      showUsage(argv[0]);
    } else {
      std::cerr << "Unexpected argument " << argv[i] << std::endl;
      showUsage(argv[0]);
    }
  }
  auto gpsToUAVRecv    = pirateReceiver<Position>(0, gpsToUAVPath, "gpsToUAV");    // Green to orange
  auto uavToTargetSend = pirateSender<Position>(1, uavToTargetPath, "uavToTarget");  // Orange to green
  auto rfToTargetSend  = pirateSender<Distance>(2, rfToTargetPath, "rfToTarget");   // Orange to green


  print([](std::ostream& o) { o << "setupUAV" << std::endl; });
  setupUAV(uavToTargetSend, gpsToUAVRecv);
  print([](std::ostream& o) { o << "setupRfSensor" << std::endl; });
  setupRfSensor(rfToTargetSend);
  print([](std::ostream& o) { o << "orange sleep" << std::endl; });
  usleep(5 * 1000 * 1000);
  return 0;
}
