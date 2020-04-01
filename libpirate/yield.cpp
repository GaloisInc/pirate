#include "libpirate.h"
#include "libpirate.hpp"

#include <vector>

typedef std::vector<void*> pirate_listeners_t;

pirate_listeners_t gaps_listeners[PIRATE_NUM_CHANNELS];

int pirate::internal::cooperative_register(int gd, void* listener) {
    gaps_listeners[gd].push_back(listener);
    return 0;
}
