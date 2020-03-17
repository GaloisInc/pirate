#pragma once
#include <chrono>
#include <functional>

using TimerMsec = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;

/** Runs an event at aregular interval for a total number of minutes. */
void onTimer(TimerMsec start, std::chrono::milliseconds dur, std::chrono::milliseconds step, std::function<void(TimerMsec)> p);