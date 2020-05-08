
#include <gtest/gtest.h>

#include "libpirate.h"
#include "libpirate.hpp"

namespace GAPS {

TEST(YieldCxx, RegisterListener)
{
    int rv, gd[2];
    errno = 0;

    rv = pirate_declare_enclaves(1, "foo");
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_pipe_parse(gd, "pipe,/tmp/test_pipe_yield,src=foo,dst=foo,listener=1", O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    int a = 6;
    int b = 7;
    rv = pirate_register_listener<int>(gd[0], [&a](const int b) { a *= b; });
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_write(gd[1], &b, sizeof(b));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, (int) sizeof(b));

    ASSERT_EQ(a, 42); // meaning of life
}

}
