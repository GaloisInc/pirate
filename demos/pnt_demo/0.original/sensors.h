#pragma once
#include "pnt_data.h"
#include "observer.h"
#include <chrono>

using namespace std::chrono;
using Clock = system_clock;
using msecs = milliseconds;
using Time = time_point<Clock, msecs>;

class Sensor : public Subject
{
public:
  Sensor() {
    _now = time_point_cast<msecs>(Clock::now());
  };
  virtual void read() = 0;
  
protected:
  Time _now;
};

class GpsSensor : public Sensor
{
  Position _p;
  Velocity _v; // only used for simulation

 public:
  GpsSensor(Position const& p, Velocity const& v) : _p(p), _v(v) { }
  Position getPosition() { return _p; }
  Time getTimePoint() { return _now; }

  void read() override {
    auto now = time_point_cast<msecs>(Clock::now());
    simulate(_v, now); // we simulate position using fixed initial velocity
    _now = now;
  }
  void notify() override {
    for (auto e : _observers)
      e->update(this);
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
    notify();
  }

};

class RfSensor : public Sensor
{
  Distance _d;
  Velocity _v; // only used for simulation

 public:
  RfSensor(Distance const& d, Velocity const& v) : _d(d), _v(v) { }
  Distance getDistance() { return _d; };

  void read() override {
    auto now = time_point_cast<msecs>(Clock::now());
    simulate(_v, now);
    _now = now;
  }
  void notify() override {
    for (auto e : _observers)
      e->update(this);
  }

 private:
  void simulate(Velocity const& v, Time const& now)
  {
    auto elapsed = duration_cast<msecs>(now - _now);
    double delta = elapsed.count() / 1000.0;
    _d._dx += v._dx * delta;
    _d._dy += v._dy * delta;
    _d._dz += v._dz * delta;
    _v = v;
    notify();
  }

};

