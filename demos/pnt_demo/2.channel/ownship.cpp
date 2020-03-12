#include "ownship.h"

void OwnShip::onGpsPositionChange(const Position& p) {
  setPosition(p);
  if (_cycle != 0 && 0 == ++_cnt % _cycle) {
    print_track();
    _c.send(_track._pos);
  }
}