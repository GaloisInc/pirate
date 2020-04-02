
#include <gtest/gtest.h>

#include "libpirate.h"
#include "libpirate.hpp"

namespace GAPS {

TEST(PirateCxx, RegisterListener)
{
    int rv, gd[2];
    pirate_channel_param_t param;
    errno = 0;
    
    rv = pirate_parse_channel_param("pipe,/tmp/test_pipe_yield", &param);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    param.yield = 1;

    rv = pirate_pipe_param(gd, &param, O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    int a = 6;
    int b = 7;
    rv = pirate_register_listener<int>(gd[0], [&a](int* b) { a *= *b; });
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_write(gd[1], &b, sizeof(b));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, (int) sizeof(b));

    ASSERT_EQ(a, 42); // meaning of life
}

}
