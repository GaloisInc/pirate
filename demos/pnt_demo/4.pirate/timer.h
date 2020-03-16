#pragma once
#include <functional>

void onTimer(unsigned total, unsigned msec, std::function<void()> p);

/** Start a thread that runs the task at a periodic interval. */
void startTimer(unsigned total, unsigned msec, std::function<void()> p);
