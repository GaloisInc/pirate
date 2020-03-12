#pragma once
#include <unordered_set>
using namespace std;

class Subject;
class Observer
{
public:
  virtual void update(Subject *) = 0;
};

class Subject
{
public:
  void attach(Observer *o) {
    _observers.insert(o);
  };
  void detach(Observer *o) {
    _observers.erase(o);
  };
  void detachAll() {
    _observers.clear();
  };
  virtual void notify() = 0;

protected:
  unordered_set<Observer *> _observers;
};

