#include "target.h"
#include "ownship.h"
#include "sensors.h"

void Target::onGpsPositionChange(const Position& p) {  
  bool tick = false;
  		
  if (_cycle != 0 && 0 == ++_cnt % _cycle) {
    targetLocation();
    print_track();
  }
}

void Target::targetLocation() {
  _track._pos._x = _uav_pos._x + _d._dx;
  _track._pos._y = _uav_pos._y + _d._dy;
  _track._pos._z = _uav_pos._z + _d._dz;
}
