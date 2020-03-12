#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <unistd.h>

// This prints out the string written to ostream in a single
// system call.
inline
void print(std::function<void(std::ostream& o)> p) {
  std::stringstream o;
  p(o);
  std::string s=o.str();
  write(STDOUT_FILENO, s.c_str(), s.size());
}

template<typename T>
using Sender = std::function<void(const T& m)>;

template<typename T>
Sender<T> fdSender(int fd) {
  return [fd](const T& d) { write(fd, &d, sizeof(T)); };
}

inline
void onTimer(int msec, std::function<void()> p)
{

  while (true)
  {
     print([](std::ostream& o) { o << "on timer" << std::endl; });
       // here we simulate sensor data streams
      p();
#ifdef _WIN32
      Sleep(sleep_msec); // 100 Hz
#else
      usleep(msec * 1000);
#endif
  }
}

/** Start a thread that runs the task at a periodic interval. */
inline
void startTimer(int msec, std::function<void()> p)
{
  print([](std::ostream& o) { o << "start timer" << std::endl; });
  std::thread(onTimer, msec, p).detach();
}

template<typename T>
using Receiver = std::function<void(std::function<void (const T& d)>)>;

template<typename T>
void readMessages(int fd, std::function<void (const T& d)> f)
{
  while (true) {
    T p;
    ssize_t cnt = read(fd, &p, sizeof(T));
    if (cnt == -1) {
      print([fd](std::ostream& o) { o << "Read " << fd << " failed " << errno << std::endl; });
      exit(-1);
    }
    if (cnt < sizeof(T)) {
      (std::cerr << "Read failed\n").flush();

      exit(-1);
    }
    f(p);
  }
}

template<typename T>
Receiver<T> fdReceiver(int fd)
{
  return [fd](std::function<void (const T& d)> f) {
    std::thread t(&readMessages<T>, fd, f);
    t.detach();
  };
}

template<typename T>
void asyncReadMessages(Receiver<T> r, std::function<void (const T& d)> f)
{
  r(f);
}
