#include <cstdio>
#include <unistd.h>
#include <sys/select.h>
#include "keyboardorientationinput.hpp"

// Debug
#include <iomanip>
#include <iostream>

KeyboardOrientationInput::KeyboardOrientationInput(
        AngularPosition<float>::UpdateCallback angPosUpdateCallback,
        float angPosMin, float angPosMax, float angIncrement) :
    OrientationInput(angPosUpdateCallback, angPosMin, angPosMax),
    mAngIncrement(angIncrement),
    mPollThread(nullptr),
    mPoll(false)
{

}

KeyboardOrientationInput::~KeyboardOrientationInput()
{
    term();
}

int KeyboardOrientationInput::init()
{
    // Setup stdin to be read one key at a time 
    int rv = tcgetattr(0, &mTermiosBackup);
    if (rv)
    {
        std::perror("tcgetattr failed");
        return -1;
    }

    struct termios raw = mTermiosBackup;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;

    rv = tcsetattr(0, TCSANOW, &raw);
    if (rv)
    {
        std::perror("tcsetattr failed");
        return -1;
    }

    // Start the reading thread
    mPoll = true;
    mPollThread = new std::thread(&KeyboardOrientationInput::pollThread, this);
    
    return 0;
}

void KeyboardOrientationInput::term()
{
    // Stop the polling thread
    if (mPollThread != nullptr)
    {
        mPoll = false;
        mPollThread->join();
        delete mPollThread;
        mPollThread = nullptr;
    }

    // Restore stdio defaults
    int rv = tcsetattr(0, TCSANOW, &mTermiosBackup);
    if (rv)
    {
        std::perror("tcsetattr failed");
    }
}

void KeyboardOrientationInput::pollThread()
{
    while (mPoll)
    {
        fd_set fdSet;
        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet); // stdin
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;

        int rv = select(1, &fdSet, NULL, NULL, &timeout);

        if (rv == -1)
        {
            std::perror("select failed");
            mPoll = false;
            return;
        } 
        else if (rv == 0)
        {
            continue;       // Timeout
        }

        char c;
        rv = read(0, &c, 1);
        if (rv != 1)
        {
            std::perror("read failed");
            mPoll = false;
            return;
        }

        float angularPosition = getAngularPosition();
        switch (c)
        {
            case LEFT:
                angularPosition += mAngIncrement;
                break;
            case RIGHT:
                angularPosition -= mAngIncrement;
                break;
            default:
                continue;
        }

        setAngularPosition(angularPosition);
    }
}

