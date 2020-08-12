#pragma once

#include <iostream>
#include "keyboardorientationinput.hpp"

#if FREESPACE_PRESENT
#include "freespaceorientationinput.hpp"
#endif

class OrientationInputCreator {
public:
    enum InputType { Freespace, Keyboard };

    static OrientationInput * get(InputType inputType, 
            AngularPosition<float>::UpdateCallback angPosUpdateCallback,
            float angPosMin = -AngularPosition<float>::DEFAULT_ANG_POS_LIMIT,
            float angPosMax =  AngularPosition<float>::DEFAULT_ANG_POS_LIMIT
            )
    {
        switch (inputType)
        {
#if FREESPACE_PRESENT
            case Freespace:
                std::cout << "Freespace is here" << std::endl;
                return new FreespaceOrientationInput(angPosUpdateCallback,
                            angPosMin, angPosMax);
#endif
            case Keyboard:
            default:
                return new KeyboardOrientationInput(angPosUpdateCallback,
                            angPosMin, angPosMax);
        }
    }
};
