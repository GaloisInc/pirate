#include "gtest/gtest.h"

#pragma pirate enclave declare(test)

int __attribute__((pirate_enclave_main("test"))) main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
