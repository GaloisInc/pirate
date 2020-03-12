// pnt_example.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <mutex>
#ifdef _WIN32
# include <Windows.h>
#else
# include <unistd.h>
#endif

#include <libpirate.h>
#include <string.h>

#include "pnt_data.h"
#include "sensors.h"
#include "ownship.h"
#include "target.h"

template<typename T>
struct Pipe {
  Sender<T> sender;
  Receiver<T> receiver;
};

template<typename T>
Pipe<T> initPipe()
{
  int fd[2];
  if (pipe(fd)) { std::cerr << "pipe failed" << std::endl; exit(-1); }
  return { .sender = fdSender<T>(fd[1]),
           .receiver = fdReceiver<T>(fd[0])
         };
}

template<typename T>
void pirateReadMessages(const char* nm, int gd, std::function<void (const T& d)> f)
{
  while (true) {
    T p;
    ssize_t cnt = pirate_read(gd, &p, sizeof(T));
    if (cnt == -1) {
      print([nm](std::ostream& o) { o << "Read " << nm << " failed " << errno << std::endl; });
      exit(-1);
    }
    if (cnt < sizeof(T)) {
      print([nm,cnt](std::ostream& o) { o << "Read " << nm << " returned " << cnt << " bytes when " << sizeof(T) << " expected" << std::endl; });
      exit(-1);
    }
    f(p);
  }
}

template<typename T>
Receiver<T> initPirateReceiver(int rd, const std::string& rdparam, const char* nm)
{
  if (rdparam == "") {
    std::cerr << "Specify " << nm << std::endl;
    exit(-1);
  }
  pirate_channel_param_t param;
  if (pirate_parse_channel_param(rdparam.c_str(), &param)) {
    std::cerr << "channel parameter set failed" << std::endl;
    exit(-1);
  }

  if (pirate_set_channel_param(rd, O_RDONLY, &param) < 0) {
    std::cerr << "channel parameter set failed" << std::endl;
    exit(-1);
  }

  print([nm](std::ostream& o) { o << nm << " receiver try open." << std::endl; });
  int rdGD = pirate_open(rd, O_RDONLY);
  if (rdGD == -1) {
    print([nm](std::ostream& o) { o << nm << " receiver open failed" << std::endl; });
    exit(-1);
  }
  print([nm](std::ostream& o) { o << nm << " receiver open." << std::endl; });

  return
    [nm, rdGD](std::function<void (const T& d)> f) {
      std::thread t(&pirateReadMessages<T>, nm, rdGD, f);
      t.detach();
    };
}

template<typename T>
Sender<T> initPirateSender(int wr, const std::string& wrparam, const char* nm)
{
  if (wrparam == "") {
    std::cerr << "Specify " << nm << std::endl;
    exit(-1);
  }

  pirate_channel_param_t param;
  if (pirate_parse_channel_param(wrparam.c_str(), &param)) {
    perror("channel parameter set failed");
    exit(-1);
  }

  if (pirate_set_channel_param(wr, O_WRONLY, &param) < 0) {
    perror("channel parameter set failed");
    exit(-1);
  }

  print([nm](std::ostream& o) { o << nm << " sender try open." << std::endl; });
  int wrGD = pirate_open(wr, O_WRONLY);

  if (wrGD == -1) {
    print([nm](std::ostream& o) { o << nm << " sender open failed" << std::endl; });
    exit(-1);
  }
  print([nm](std::ostream& o) { o << nm << " sender open." << std::endl; });


  return [nm,wrGD](const T& d) {
      int cnt = pirate_write(wrGD, &d, sizeof(T));
      print([nm](std::ostream& o) { o << "Write " << nm << " " << sizeof(T) << " bytes." << std::endl; });
      if (cnt == -1) {
        print([nm](std::ostream& o) { o << "Write to " << nm << " failed " << errno << std::endl; });
        exit(-1);
      }
      if (cnt != sizeof(T)) {
        print([nm](std::ostream& o) { o << "Write to " << nm << " had unexpected count." << std::endl; });
        exit(-1);
      }
    };
}

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

  auto gpsToTargetChan = initPipe<Position>(); // Green to green
  auto gpsToUAVSend    = initPirateSender<Position>(  0, gpsToUAVPath, "gpsToUAV");    // Green to orange
  auto uavToTargetRecv = initPirateReceiver<Position>(1, uavToTargetPath, "uavToTarget");  // Orange to green
  auto rfToTargetRecv  = initPirateReceiver<Distance>(2, rfToTargetPath, "rfToTarget");   // Orange to green

  // Setup sender for gps that broadcasts to two other channels.
  Sender<Position> gpsSender = [gpsToUAVSend, gpsToTargetChan](const Position& p) {
    gpsToUAVSend(p);
    gpsToTargetChan.sender(p);
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
  auto gpsToUAVRecv    = initPirateReceiver<Position>(0, gpsToUAVPath, "gpsToUAV");    // Green to orange
  auto uavToTargetSend = initPirateSender<Position>(1, uavToTargetPath, "uavToTarget");  // Orange to green
  auto rfToTargetSend  = initPirateSender<Distance>(2, rfToTargetPath, "rfToTarget");   // Orange to green

  setupUAV(uavToTargetSend, gpsToUAVRecv);
  setupRfSensor(rfToTargetSend);
  usleep(5 * 1000 * 1000);
  return 0;
}
