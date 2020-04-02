
#include <gtest/gtest.h>

#include "libpirate.h"
#include "libpirate.hpp"

namespace GAPS {

TEST(PirateCxx, RegisterListener)
{
    pirate_options_t options;
    pirate_init_options(&options);
    options.yield = 1;
    pirate_set_options(&options);

    int gd[2];
    int rv = pirate_pipe_parse(gd, "pipe,/tmp/test_pipe_yield", O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    int a = 6;
    int b = 7;
    rv = pirate_register_listener<int>(gd[0], [&a](int* b) { a *= *b; });
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_write(gd[1], &b, sizeof(b));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, sizeof(b));

    ASSERT_EQ(a, 42); // meaning of life
    options.yield = 0;
    pirate_set_options(&options);    
}

}
