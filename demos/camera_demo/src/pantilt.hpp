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

#pragma once

#include <iostream>

class PanTilt
{
    public:
        float pan;
        float tilt;

        PanTilt(): pan(0.0), tilt(0.0) { }
        PanTilt(float pan, float tilt): pan(pan), tilt(tilt) { }

    PanTilt& operator=(const PanTilt& other) {
        if (this != &other) {
            this->pan = other.pan;
            this->tilt = other.tilt;
        }
        return *this;
    }

    PanTilt& operator+=(const PanTilt& other) {
        this->pan += other.pan;
        this->tilt += other.tilt;
        return *this;
    }

};

std::ostream& operator<<(std::ostream& out, const PanTilt& p);

bool operator==(const PanTilt &pt1, const PanTilt &pt2);
 
bool operator!=(const PanTilt &pt1, const PanTilt &pt2);
