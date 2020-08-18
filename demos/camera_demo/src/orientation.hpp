#pragma once

#include <functional>

template <class T>
class AngularPosition
{
public:
    AngularPosition(T angPosMin = -DEFAULT_ANG_POS_LIMIT,
                    T angPosMax =  DEFAULT_ANG_POS_LIMIT) :
        mAngularPositionMin(angPosMin),
        mAngularPositionMax(angPosMax),
        mAngularPosition(0)
    {

    }

    virtual ~AngularPosition()
    {

    }

    virtual T getAngularPosition() const
    {
        return mAngularPosition;   
    }

    virtual bool setAngularPosition(T angularPosition)
    {
        if (angularPosition < mAngularPositionMin)
        {
            angularPosition = mAngularPositionMin;
        }
        else if (angularPosition > mAngularPositionMax)
        {
            angularPosition = mAngularPositionMax;
        }

        if (mAngularPosition != angularPosition)
        {
            mAngularPosition = angularPosition;
            return true;
        }

        return false;
    }

    static constexpr T DEFAULT_ANG_POS_LIMIT = 90;

    using UpdateCallback = std::function<bool(T)>;
protected:
    const T mAngularPositionMin;
    const T mAngularPositionMax;
private:
    T mAngularPosition;
};

