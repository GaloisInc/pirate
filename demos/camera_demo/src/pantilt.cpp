/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include "pantilt.hpp"

std::ostream& operator<<(std::ostream& out, const PanTilt& pt) {
    out << '(' << pt.pan
        << ',' << pt.tilt
        << ')';
    return out;
}

bool operator==(const PanTilt &pt1, const PanTilt &pt2) {
    return ((pt1.pan == pt2.pan) && (pt1.tilt == pt2.tilt));
}
 
bool operator!=(const PanTilt &pt1, const PanTilt &pt2) {
    return !(pt1 == pt2);
}
