#pragma once
#include <chrono>
#include <functional>

using TimerMsec = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;

/** Runs an event at aregular interval for a total number of minutes. */
void onTimer(TimerMsec start, std::chrono::milliseconds dur, std::chrono::milliseconds step, std::function<void(TimerMsec)> p);

/** This is a class for advancing time by a fixed amount each request. */
template<typename T>
class FixedClock {
  std::chrono::time_point<std::chrono::steady_clock, T> base;
  T _increment;
public:
  FixedClock(T inc) : _increment(inc) {}

  std::chrono::time_point<std::chrono::steady_clock, T> operator()(void) {
    auto now = base;
    base = base + _increment;
    return now;
  }
};

template<typename T>
std::function<std::chrono::time_point<std::chrono::steady_clock, T>()>
fixedClock(T inc) {
    return FixedClock<T>(inc);
};

template<typename T>
std::chrono::time_point<std::chrono::steady_clock, T> systemClock(void) {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
};