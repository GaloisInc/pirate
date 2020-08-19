#pragma once

// Definition of a simple interface for reading camera angular orientation

#include "orientation.hpp"

class OrientationInput : public AngularPosition<float>
{
public:
    OrientationInput(   AngularPosition<float>::UpdateCallback updateCallback,
                        float angPosMin = -AngularPosition<float>::DEFAULT_ANG_POS_LIMIT, 
                        float angPosMax =  AngularPosition<float>::DEFAULT_ANG_POS_LIMIT) :
        AngularPosition(angPosMin, angPosMax),
        mUpdateCallback(updateCallback)
        
    {

    }

    virtual ~OrientationInput()
    {

    }

    virtual int init() = 0;
    virtual void term() = 0;

    virtual bool setAngularPosition(float angularPosition) override
    {
        bool updated = AngularPosition::setAngularPosition(angularPosition);
        if (updated)
        {
            mUpdateCallback(getAngularPosition());
        }

        return updated;
    }
private:
    AngularPosition<float>::UpdateCallback mUpdateCallback;
};

