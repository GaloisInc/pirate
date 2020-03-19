#include "target.h"
#include "ownship.h"
#include "sensors.h"

void Target::setDistance(Distance const& d) {
  _d_cnt++;
  _d = d;
  targetLocation();
}

void Target::setUAVLocation(Position const& p) {
  _uav_pos_cnt++;
  _uav_pos = p;
  targetLocation();
}

void Target::targetLocation() {
  if ((_uav_pos_cnt == _d_cnt) && (0 == ++_cnt % _cycle)) {
    _track._pos._x = _uav_pos._x + _d._dx;
    _track._pos._y = _uav_pos._y + _d._dy;
    _track._pos._z = _uav_pos._z + _d._dz;
    print_track();
  }
}
