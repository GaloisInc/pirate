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


int main()
{
  Position p(.0, .0, .0); // initial position
  Distance d(1062, 7800, 9000); // initial target distance

  Velocity v(50, 25, 12);
  Velocity vtgt(35, 625, 18);
  
  Time now;

  CallbackChannel<Position> gpsData;
  CallbackChannel<Distance> rfData;
  CallbackChannel<Position> uavData;

  GpsSensor* gps = new GpsSensor(&gpsData, p, v);
  RfSensor* rfs  = new RfSensor(&rfData, d, vtgt);
  OwnShip* uav   = new OwnShip(uavData, 100); // updates at 100 Hz frequency
  Target* tgt    = new Target(10); // updates at 10 Hz frequency

  // setup the dataflow relationships
  gpsData.addListener([uav](const Position& p) { uav->onGpsPositionChange(p); });
  gpsData.addListener([tgt](const Position& p) { tgt->onGpsPositionChange(p); });
  uavData.addListener([tgt](const Position& p) { tgt->setUAVLocation(p); });
  rfData.addListener( [tgt](const Distance& d) { tgt->setDistance(d); });

  while (true)
    {
      // here we simulate sensor data streams
      gps->read(msecs(10));
      rfs->read(msecs(10));
      
#ifdef _WIN32	  
      Sleep(sleep_msec); // 100 Hz
#else
      usleep(sleep_msec * 1000);
#endif
    }
  return 0;
}

