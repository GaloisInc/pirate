#pragma once

// Angular position is read from the keyboard left/right keys

#include <thread>
#include <termios.h>

#include "orientationinput.hpp"

class KeyboardOrientationInput : public OrientationInput
{
public:
    KeyboardOrientationInput(AngularPosition<float>::UpdateCallback angPosUpdateCallback,
                             float angPosMin = -AngularPosition<float>::DEFAULT_ANG_POS_LIMIT,
                             float angPosMax =  AngularPosition<float>::DEFAULT_ANG_POS_LIMIT,
                             float angIncrement = DEFAULT_ANG_INCR);
    virtual ~KeyboardOrientationInput();

    virtual int init();
    virtual void term();

    static constexpr float DEFAULT_ANG_INCR = 
        AngularPosition<float>::DEFAULT_ANG_POS_LIMIT / 30.0;
private:
    const float mAngIncrement;

    enum eKeyCode {
        LEFT  = 0x44,
        RIGHT = 0x43
    };

    struct termios mTermiosBackup;

    std::thread *mPollThread;
    bool mPoll;
    void pollThread();
};

