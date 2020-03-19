#pragma once
#include "channel.h"
#include "pnt_data.h"
#include <iostream>
#include <mutex>

class Target
{
private:
  std::mutex _mutex;
  Distance  _d;
  Position _uav_pos;
  Track _track;
  int _frequency;
  int _cycle;
  int _cnt, _d_cnt, _uav_pos_cnt;

public:
  Target(int rate = 1) : _frequency(rate), _cnt(0), _d_cnt(0), _uav_pos_cnt(0) {
    _cycle = static_cast<int> (((1.0 / _frequency) / (sleep_msec / 1000)));
  };

  ~Target() {};

  Track getTracking() { return _track; }

  void print_track() {
    print([this](std::ostream& o) {
      o << "\t\t--- Target TRACK ---" << std::endl
	      << "\t\t x=" << _track._pos._x << std::endl
	      << "\t\t y=" << _track._pos._y << std::endl
	      << "\t\t z=" << _track._pos._z << std::endl << std::endl;
    });
  }
  void setDistance(Distance const& d);
  void setUAVLocation(Position const& p);
private:
  void targetLocation();
};
