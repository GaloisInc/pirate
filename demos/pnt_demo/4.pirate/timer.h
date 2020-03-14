#pragma once
#include <functional>

/** Start a thread that runs the task at a periodic interval. */
void startTimer(int msec, std::function<void()> p);
