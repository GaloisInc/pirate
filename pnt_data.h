#pragma once
enum sensitivity { red, blue, green, yellow, orange };
static int max_fqc = 100;
static double sleep_msec = 1000 / max_fqc; 

struct Distance {
  Distance() :_dx(.0), _dy(.0), _dz(.0) {};
  Distance(double dx, double dy, double dz) :_dx(dx), _dy(dy), _dz(dz) {};

  double _dx;
  double _dy;
  double _dz;
};


struct Position {
  Position() :_x(.0), _y(.0), _z(.0) {};
  Position (double x, double y, double z) :_x(x), _y(y), _z(z) {};

  double _x;
  double _y;
  double _z;
};


struct Velocity {
  Velocity() :_dx(.0), _dy(.0), _dz(.0) {};
  Velocity(double dx, double dy, double dz) :_dx(dx), _dy(dy), _dz(dz) {};

  double _dx;
  double _dy;
  double _dz;
};

struct Track {
  Position _pos;
  Velocity _v;
};


