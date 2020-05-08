
#include <gtest/gtest.h>

#include "libpirate.h"
#include "libpirate.hpp"

namespace GAPS {

TEST(Yield, ControlChannel)
{
    int rv, gd[2];
    uint8_t status;
    errno = 0;

    rv = pirate_declare_enclaves(1, "foo");
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_pipe_parse(gd, "pipe,/tmp/test_pipe_yield,src=foo,dst=foo,control=1", O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_yield("foo");
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_read(gd[0], &status, sizeof(status));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ((unsigned long) rv, sizeof(status));    
}

}
