#pragma once

#include <iostream>
#include "keyboardorientationinput.hpp"

#if FREESPACE_PRESENT
#include "freespaceorientationinput.hpp"
#endif

#include "options.hpp"

class OrientationInputCreator {
public:
    static OrientationInput * get(
            AngularPosition<float>::UpdateCallback angPosUpdateCallback,
            Options& options)
    {
        switch (options.mInputType)
        {
#if FREESPACE_PRESENT
            case Freespace:
                std::cout << "Freespace is here" << std::endl;
                return new FreespaceOrientationInput(angPosUpdateCallback,
                            -options.mAngularPositionLimit,
                            options.mAngularPositionLimit);
#endif
            case Keyboard:
            default:
                return new KeyboardOrientationInput(angPosUpdateCallback,
                            -options.mAngularPositionLimit,
                            options.mAngularPositionLimit);
        }
    }
};
