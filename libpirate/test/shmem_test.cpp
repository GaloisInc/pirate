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

#include "primitives.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;

TEST(ChannelShmemTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    // Default configuration
    pirate_channel_param_t param;
    int rv = pirate_init_channel_param(SHMEM, channel, flags, &param);
#if PIRATE_SHMEM_FEATURE
    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SHMEM_NAME, channel);

    pirate_shmem_param_t *shmem_param = &param.shmem;
    ASSERT_STREQ(default_path, shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, shmem_param->buffer_size);

    // Apply configuration
    const char *path = "/tmp/gaps_shmem_test";
    const unsigned buffer_size = DEFAULT_SMEM_BUF_LEN * 2;

    strncpy(shmem_param->path, path, sizeof(shmem_param->path) - 1);
    shmem_param->buffer_size = buffer_size;

    rv = pirate_set_channel_param(SHMEM, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_shmem_param_t *shmem_param_get = &param_get.shmem;
    memset(shmem_param_get, 0, sizeof(*shmem_param_get));

    channel_t ch =  pirate_get_channel_param(channel, O_RDONLY, &param_get);
    ASSERT_EQ(SHMEM, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, shmem_param_get->path);
    ASSERT_EQ(buffer_size, shmem_param_get->buffer_size);
#else
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(ESOCKTNOSUPPORT, errno);
    errno = 0;
#endif
}

TEST(ChannelShmemTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    channel_t channel;

    char opt[128];
    const char *name = "shmem";
    const char *path = "/tmp/test_shmem";
    const uint32_t buffer_size = 42 * 42;

#if PIRATE_SHMEM_FEATURE
    const pirate_shmem_param_t *shmem_param = &param.shmem;
    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SHMEM_NAME, ch_num);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, shmem_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, shmem_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, shmem_param->path);
    ASSERT_EQ(buffer_size, shmem_param->buffer_size);
#else
    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(INVALID, channel);
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
        int rv = pirate_init_channel_param(SHMEM, Writer.channel, O_WRONLY,
                                            &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        unsigned buffer_size = GetParam();
        if (buffer_size) {
            param.shmem.buffer_size = buffer_size;
        }

        rv = pirate_set_channel_param(SHMEM, Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(SHMEM, Reader.channel, O_RDONLY, &param);
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
