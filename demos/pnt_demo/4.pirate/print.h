#pragma once
#include <functional>
#include <iostream>

// This prints out the string written to ostream in a single
// system call so that it is atomic.
//void print(std::function<void(std::ostream& o)> p);

// This prints out the contents written to ostream in a single
// system call so that it is atomic.
void channel_errlog(std::function<void(FILE*)> p);