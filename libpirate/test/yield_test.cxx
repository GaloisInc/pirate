
#include <gtest/gtest.h>

#include "libpirate.hpp"

namespace GAPS {

TEST(PirateCxx, RegisterListener)
{
    pirate::listener_register<int>(1, [](const int&) { });
}

}
