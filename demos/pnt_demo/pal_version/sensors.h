#pragma once
#include "channel.h"
#include "pnt_data.h"
#include <chrono>
#include "print.h"

#define CAPABILITY(c) __attribute__((pirate_capability(c)))

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

#pragma pirate capability declare(gps)

template<typename T>
void doNothing(const T&) {}

class GpsSensor : public Sensor
{
  Position _p;
  Velocity _v; // only used for simulation

public:
  CAPABILITY("gps")
  GpsSensor(const Time& now, Position const& p, Velocity const& v)
  : Sensor(now), _p(p), _v(v), onPositionUpdate(doNothing<Position>) { }
  Position getPosition() { return _p; }
  Time getTimePoint() { return _now; }

  std::function<void(const Position&)> onPositionUpdate;

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
    onPositionUpdate(_p);
  }
};

#pragma pirate capability declare(rfsensor)

class RfSensor : public Sensor
{
  Distance _d;
  Velocity _v; // only used for simulation

 public:
  CAPABILITY("rfsensor")
  RfSensor(const Time& now, Distance const& d, Velocity const& v) : Sensor(now), _d(d), _v(v), onDistanceUpdate(doNothing<Distance>) { }
  Distance getDistance() { return _d; };

  std::function<void(const Distance&)> onDistanceUpdate;

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
    onDistanceUpdate(_d);
  }
};