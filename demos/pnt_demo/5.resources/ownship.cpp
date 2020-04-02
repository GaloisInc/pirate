#include "ownship.h"

void OwnShip::onGpsPositionChange(const Position& p) {
  static int cnt = 0;
  setPosition(p);
  if (_cycle != 0 && 0 == ++cnt % _cycle) {
    print_track();
    _c(_track._pos);
  }
}
