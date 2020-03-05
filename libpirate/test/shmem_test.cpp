/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;


TEST(ChannelShmemTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;

    char opt[128];
    const char *name = "shmem";
    const char *path = "/tmp/test_shmem";
    const uint32_t buffer_size = 42 * 42;

#if PIRATE_SHMEM_FEATURE
    const pirate_shmem_param_t *shmem_param = &param.shmem;
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SHMEM, param.channel_type);
    ASSERT_STREQ("", shmem_param->path);
    ASSERT_EQ((unsigned)0, shmem_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SHMEM, param.channel_type);
    ASSERT_STREQ(path, shmem_param->path);
    ASSERT_EQ((unsigned)0, shmem_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SHMEM, param.channel_type);
    ASSERT_STREQ(path, shmem_param->path);
    ASSERT_EQ(buffer_size, shmem_param->buffer_size);
#else
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(ESOCKTNOSUPPORT, errno);
    errno = 0;
#endif
}

#if PIRATE_SHMEM_FEATURE
class ShmemTest : public ChannelTest, public WithParamInterface<int>
{
public:
    void ChannelInit()
    {
        int rv;
        pirate_init_channel_param(SHMEM, &param);
        unsigned buffer_size = GetParam();
        if (buffer_size) {
            param.shmem.buffer_size = buffer_size;
        }

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const int TEST_BUF_LEN = DEFAULT_SMEM_BUF_LEN / 2;
};

TEST_P(ShmemTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(ShmemFunctionalTest, ShmemTest,
    Values(0, ShmemTest::TEST_BUF_LEN));
#endif

} // namespace
