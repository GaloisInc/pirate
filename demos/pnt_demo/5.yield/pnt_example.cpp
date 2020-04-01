// pnt_example.cpp : Defines the entry point for the console application.
//

#include "ownship.h"
#include "pnt_data.h"
#include "print.h"
#include "sensors.h"
#include "target.h"

#include <string.h>
#include <unistd.h>

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

int open_channel(std::string config, int flags) {
  int gd = pirate_open_parse(config.c_str(), flags);
  if (gd < 0) {
      channel_errlog([config](FILE* f) { fprintf(f, "Open %s failed (error = %d)", config.c_str(), errno); });
      exit(-1);
  }
  return gd;
}

void send_control(int gd) {
  unsigned char ctrl = 0;
  pirate_write(gd, &ctrl, sizeof(ctrl));
}

int run_green(int argc, char** argv) PIRATE_ENCLAVE_MAIN("green")
{
  // Parse command line arguments
  std::string gpsToUAVPath;
  std::string gpsToTargetPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
  std::string greenToOrangePath;
  std::string orangeToGreenPath;
  int duration;
  int i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--gps-to-uav") == 0) {
      gpsToUAVPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--gps-to-target") == 0) {
      gpsToTargetPath = argv[i+1];
      i+=2;      
    } else if (strcmp(argv[i], "--uav-to-target") == 0) {
      uavToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--rf-to-target") == 0) {
      rfToTargetPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--green-to-orange-control") == 0) {
      greenToOrangePath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--orange-to-green-control") == 0) {
      orangeToGreenPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--duration") == 0) {
      duration = atoi(argv[i+1]);
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

  int gpsTargetGd[2];

  int rv = pirate_pipe_parse(gpsTargetGd, gpsToTargetPath.c_str(), O_RDWR);
  if (rv < 0) {
      exit(-1);
  }
  int gpsUavGd = open_channel(gpsToUAVPath, O_WRONLY);
  int uavGd = open_channel(uavToTargetPath, O_RDONLY);
  int rfGd = open_channel(rfToTargetPath, O_RDONLY);
  int readCtrlGd = open_channel(orangeToGreenPath, O_RDONLY);
  int writeCtrlGd = open_channel(greenToOrangePath, O_WRONLY);

  GreenListener listener(uavGd, rfGd, gpsTargetGd[0], readCtrlGd, writeCtrlGd);

  // CreateGPS
  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);
  SendChannel<Position> gpsSender(listener);
  GpsSensor gps(gpsSender, p, v);
  gpsSender.add(gpsUavGd);
  gpsSender.add(gpsTargetGd[1]);

  Target tgt(10); // updates at 10 Hz frequency

  listener._uavFunc = [&tgt](const Position& p) { tgt.setUAVLocation(p); };
  listener._rfFunc = [&tgt](const Distance& d) { tgt.setDistance(d); };
  listener._gpsFunc = [&tgt](const Position& p) { tgt.onGpsPositionChange(p); };

  for(int i = 0; i < duration / 10; i++) {
      // here we simulate sensor data streams
      gps.read(msecs(10));
#ifdef _WIN32	  
      Sleep(sleep_msec); // 100 Hz
#else
      usleep(sleep_msec * 500);
#endif
      // send control to the orange enclave
      send_control(writeCtrlGd);
      // wait for control from the orange enclave
      listener.listen();
  }

  return 0;
}

int run_orange(int argc, char** argv) PIRATE_ENCLAVE_MAIN("orange")
{
  // Parse command line arguments
  std::string gpsToUAVPath;
  std::string uavToTargetPath;
  std::string rfToTargetPath;
  std::string greenToOrangePath;
  std::string orangeToGreenPath;
  int duration;
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
    } else if (strcmp(argv[i], "--green-to-orange-control") == 0) {
      greenToOrangePath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--orange-to-green-control") == 0) {
      orangeToGreenPath = argv[i+1];
      i+=2;
    } else if (strcmp(argv[i], "--duration") == 0) {
      duration = atoi(argv[i+1]);
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

  int gpsGd = open_channel(gpsToUAVPath, O_RDONLY);
  int uavGd = open_channel(uavToTargetPath, O_WRONLY);
  int rfGd = open_channel(rfToTargetPath, O_WRONLY);
  int writeCtrlGd = open_channel(orangeToGreenPath, O_WRONLY);
  int readCtrlGd = open_channel(greenToOrangePath, O_RDONLY);

  OrangeListener listener(gpsGd, readCtrlGd, writeCtrlGd);

  SendChannel<Distance> rfSender(listener);
  SendChannel<Position> uavSender(listener);
  rfSender.add(rfGd);
  uavSender.add(uavGd);

  // Create RF sensor
  Distance dtgt(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor rfs(rfSender, dtgt, vtgt);

  // Create UAV and have it start listening.
  OwnShip uav(uavSender, 100); // updates at 100 Hz frequency

  listener._gpsFunc = [&uav](const Position& p) { uav.onGpsPositionChange(p); };

  for (int i = 0; i < duration / 10; i++) {
      // wait for control from the green enclave
      listener.listen();
      // here we simulate sensor data streams
      rfs.read(msecs(10));
#ifdef _WIN32	  
      Sleep(sleep_msec); // 100 Hz
#else
      usleep(sleep_msec * 500);
#endif
    // send control to the green enclave
      send_control(writeCtrlGd);
  }

  return 0;
}