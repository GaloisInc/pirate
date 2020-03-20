#pragma once
#include "channel.h"
#include "pnt_data.h"
#include <chrono>
#include "print.h"

using namespace std::chrono;
using Clock = system_clock;
using msecs = milliseconds;
using Time = std::chrono::time_point<std::chrono::steady_clock, msecs>;

class Sensor
{
public:
  Sensor(const Time& now) {
    _now = time_point_cast<msecs>(now);
  };
  virtual void read(const Time& now) = 0;
  
protected:
  Time _now;
};

class GpsSensor : public Sensor
{
  Position _p;
  Velocity _v; // only used for simulation

 public:
  GpsSensor(const Time& now, Position const& p, Velocity const& v) : Sensor(now), _p(p), _v(v) { }
  Position getPosition() { return _p; }
  Time getTimePoint() { return _now; }

  void read(const Time& now) override {
    simulate(_v, now); // we simulate position using fixed initial velocity
    _now = now;
  }

 private:
  void simulate(Velocity const& v, Time const& now)
  {
    auto elapsed = duration_cast<msecs>(now - _now);
    double delta = elapsed.count() / 1000.0;
    _p._x += v._dx * delta;
    _p._y += v._dy * delta;
    _p._z += v._dz * delta;
    _v = v;
  }
};

class RfSensor : public Sensor
{
  Sender<Distance> _c; 
  Distance _d;
  Velocity _v; // only used for simulation

 public:
  RfSensor(const Time& now, Sender<Distance> c, Distance const& d, Velocity const& v) : Sensor(now), _c(c), _d(d), _v(v) { }
  Distance getDistance() { return _d; };

  void read(const Time& now) override {
    simulate(_v, now);
    _now = now;
  }

  void simulate(Velocity const& v, Time const& now)
  {
    auto elapsed = duration_cast<msecs>(now - _now);
    double delta = elapsed.count() / 1000.0;
    _d._dx += v._dx * delta;
    _d._dy += v._dy * delta;
    _d._dz += v._dz * delta;
    _v = v;
    _c(_d);
  }
};

