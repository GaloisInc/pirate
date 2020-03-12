#pragma once
#include "pnt_data.h"
#include "observer.h"
#include <chrono>

using namespace std::chrono;
using Clock = system_clock;
using msecs = std::chrono::milliseconds;
using Time = std::chrono::time_point<std::chrono::system_clock, msecs>;

class Sensor : public Subject
{
public:
  Sensor(const Time& now) : _now(now) {
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
  RfSensor(const Time& now, Distance const& d, Velocity const& v) : Sensor(now), _d(d), _v(v) { }
  Distance getDistance() { return _d; };

  void read(const Time& now) override {
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

