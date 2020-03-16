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

  //auto gpsToTargetChan = anonPipe<Position>(); // Green to green
  //auto gpsToUAVSend    = unixSeqPacketSender<Position>(gpsToUAVPath);    // Green to orange
  //auto uavToTargetRecv = unixSeqPacketReceiver<Position>(uavToTargetPath);  // Orange to green
  //auto rfToTargetRecv  = unixSeqPacketReceiver<Distance>(rfToTargetPath);   // Orange to green
  //auto gpsToTargetSend = gpsToTargetChan.sender;

  // Setup sender for gps that broadcasts to two other channels.
  Sender<Position> gpsSender = [](const Position& p) {
    channel_errlog([](FILE* f) { fprintf(f, "About to send\n");});
    //gpsToUAVSend(p);
    //gpsToTargetSend(p);
  };

  Target* tgt = new Target(10); // updates at 10 Hz frequency
  //auto tgtMutex = new std::mutex();

/*
  asyncReadMessages<Position>(uavToTargetRecv,
    [tgt, tgtMutex](const Position& p) {
        std::lock_guard<std::mutex> g(*tgtMutex);
        tgt->setUAVLocation(p);
      });*/
      /*
  std::thread rfToTargetThread(rfToTargetRecv,
    [tgt](const Distance& d) {
        channel_errlog([](FILE* f) { fprintf(f, "Received distance\n"); });
     //   std::lock_guard<std::mutex> g(*tgtMutex);
        tgt->setDistance(d);
      });
      */
      /*
  asyncReadMessages<Position>(gpsToTargetChan.receiver,
    [tgt, tgtMutex](const Position& p) {
      std::lock_guard<std::mutex> g(*tgtMutex);
      tgt->onGpsPositionChange(p);
    });*/


  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);

  GpsSensor* gps = new GpsSensor(gpsSender, p, v);
  // Run every 10 gps milliseconds.
  /*
  std::thread gpsThread(onTimer, 50, 10, [gps](){
      print([](std::ostream& o) { o << "gps thread" << std::endl; });
      gps->read(msecs(10));
      print([](std::ostream& o) { o << "gps thread done" << std::endl; });
    });
  gpsThread.join();
  */
  //rfToTargetThread.join();
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
//  auto gpsToUAVRecv    = unixSeqPacketReceiver<Position>(gpsToUAVPath);    // Green to orange
  //auto uavToTargetSend = unixSeqPacketSender<Position>(uavToTargetPath);  // Orange to green
  //auto rfToTargetSend  = unixSeqPacketSender<Distance>(rfToTargetPath);   // Orange to green

  auto rfToTargetSend = [](const Distance& p) {
    channel_errlog([](FILE* f) { fprintf(f, "About to send\n");});
    //gpsToUAVSend(p);
    //gpsToTargetSend(p);
  };

  print([](std::ostream& o) { o << "setupUAV" << std::endl; });
  //OwnShip* uav = new OwnShip(uavToTargetSend, 100); // updates at 100 Hz frequency

/*
  asyncReadMessages<Position>(gpsToUAVRecv,
    [](const Position& p) {
//        uav->onGpsPositionChange(p);
      });
      */
  print([](std::ostream& o) { o << "setupRfSensor" << std::endl; });

  Distance dtgt(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor* rfs  = new RfSensor(rfToTargetSend, dtgt, vtgt);

  std::thread rfThread(onTimer, 1000, 10, [rfs]() {
    print([](std::ostream& o) { o << "rf sensor event." << std::endl; });
    rfs->read(msecs(10));
    print([](std::ostream& o) { o << "rf sensor event done" << std::endl; });
  });
  rfThread.join();
  return 0;
}
