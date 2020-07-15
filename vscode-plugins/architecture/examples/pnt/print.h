#pragma once
#include <functional>
#include <stdio.h>

// This prints out the contents written to ostream in a single
// system call so that it is atomic.
void channel_errlog(std::function<void(FILE*)> p);