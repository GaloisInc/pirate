// pnt_example.cpp : Defines the entry point for the console application.
//

#include <iostream>
#ifdef _WIN32
# include <Windows.h>
#else
# include <unistd.h>
#endif
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
  return { .sender = fdSender<T>(fd[1]), .receiver = { .fd = fd[0]} };
}

void setupGps(Sender<Position> gpsSender)
{
  Position p(.0, .0, .0); // initial position
  Velocity v(50, 25, 12);

  GpsSensor* gps = new GpsSensor(gpsSender, p, v);  
  startTimer(10, [gps](){ gps->read(msecs(10)); });
}

void setupTarget(Receiver<Position> uav, Receiver<Distance> rf, Receiver<Position> gps)
{
  
  Target* tgt    = new Target(10); // updates at 10 Hz frequency
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

// This doesn't reurn ans must be last.
void setupRfSensor(Sender<Distance> rfSender)
{
  Distance d(1062, 7800, 9000); // initial target distance
  Velocity vtgt(35, 625, 18);
  RfSensor* rfs  = new RfSensor(rfSender, d, vtgt);
  onTimer(sleep_msec, [rfs]() { rfs->read(msecs(10)); });
}

void setupUAV(Sender<Position> uavSender, Receiver<Position> gps)
{
  OwnShip* uav = new OwnShip(uavSender, 100); // updates at 100 Hz frequency  
  asyncReadMessages<Position>(gps, 
    [uav](const Position& p) { 
        uav->onGpsPositionChange(p); 
      });
}

int main()
{
  auto gpsToTargetChan = initPipe<Position>(); // Green to green
  auto gpsToUAVChan    = initPipe<Position>(); // Green to yellow
  auto uavToTargetChan = initPipe<Position>(); // Yellow to green
  auto rfToTargetChan  = initPipe<Distance>(); // Yellow to green

  // Setup sender for gps that broadcasts to two other channels.
  Sender<Position> gpsSender = [gpsToUAVChan, gpsToTargetChan](const Position& p) {
    gpsToUAVChan.sender(p);
    gpsToTargetChan.sender(p);
  };

  setupTarget(uavToTargetChan.receiver, rfToTargetChan.receiver, gpsToTargetChan.receiver);
  setupUAV(uavToTargetChan.sender, gpsToUAVChan.receiver);
  setupGps(gpsSender);
  setupRfSensor(rfToTargetChan.sender);
  return 0;
}