#pragma once
#include "channel.h"
#include "pnt_data.h"
#include <chrono>

using namespace std::chrono;
using Clock = system_clock;
using msecs = milliseconds;
using Time = std::chrono::time_point<std::chrono::system_clock, msecs>;

class GpsSensor
{
  Sender<Position> _c;
  Position _p;
  Velocity _v; // only used for simulation

 public:
  GpsSensor(Sender<Position> c, Position const& p, Velocity const& v) : _c(c), _p(p), _v(v) { }
  Position getPosition() { return _p; }

  void read(const msecs& elapsed) {
    double delta = elapsed.count() / 1000.0;
    _p._x += _v._dx * delta;
    _p._y += _v._dy * delta;
    _p._z += _v._dz * delta;
    _c(_p);
  }
};

class RfSensor
{
  Sender<Distance> _c; 
  Distance _d;
  Velocity _v; // only used for simulation

 public:
  RfSensor(Sender<Distance> c, Distance const& d, Velocity const& v) : _c(c), _d(d), _v(v) { }
  Distance getDistance() { return _d; };

  void read(const msecs& elapsed) {
    double delta = elapsed.count() / 1000.0;
    _d._dx += _v._dx * delta;
    _d._dy += _v._dy * delta;
    _d._dz += _v._dz * delta;
    _c(_d);
  }
};

