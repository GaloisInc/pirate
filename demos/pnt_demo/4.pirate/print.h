#pragma once
#include <functional>
#include <iostream>

// This prints out the string written to ostream in a single
// system call.
void print(std::function<void(std::ostream& o)> p);

// This prints out the string written to ostream in a single
// system call.
void channel_error(std::function<void(std::ostream& o)> p);