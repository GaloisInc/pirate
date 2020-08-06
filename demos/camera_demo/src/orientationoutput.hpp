#pragma once

#include "orientation.hpp"

using OrientationUpdateCallback = AngularPosition<float>::UpdateCallback;

class OrientationOutput : public AngularPosition<float>
{
public:
    OrientationOutput(float angularPositionLimit = DEFAULT_ANG_POS_LIMIT);
    virtual ~OrientationOutput();

    virtual int init();
    virtual void term();

    virtual bool setAngularPosition(float angularPosition) override;

    const OrientationUpdateCallback& getUpdateCallback(); 
private:
    static constexpr float DEFAULT_ANG_POS_LIMIT = 90.0;

    const OrientationUpdateCallback mUpdateCallback;
};

