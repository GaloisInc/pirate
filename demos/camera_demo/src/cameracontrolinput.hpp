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

// Definition of a simple interface for reading camera control parameters

#include "cameracontroloutput.hpp"

class CameraControlInput
{
public:
    CameraControlInput(CameraControlCallbacks cameraControlCallbacks) :
        mCallbacks(cameraControlCallbacks)
    {

    }

    virtual ~CameraControlInput()
    {

    }

    virtual int init() = 0;
    virtual void term() = 0;

protected:
    CameraControlCallbacks mCallbacks;
};
