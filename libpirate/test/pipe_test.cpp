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

#include <errno.h>
#include "primitives.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;

TEST(ChannelPipeTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    char default_path[128];
    snprintf(default_path, PIRATE_LEN_NAME - 1, PIRATE_PIPE_NAME_FMT, channel);

    // Default configuration
    pirate_channel_param_t param;
    pirate_pipe_param_t *pipe_param = &param.pipe;
    int rv = pirate_init_channel_param(PIPE, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, pipe_param->path);
    ASSERT_EQ((unsigned)0, pipe_param->iov_len);

    // Apply configuration
    const char *test_path = "/tmp/test_path";
    const unsigned iov_len = 42;
    strncpy(pipe_param->path, test_path, sizeof(pipe_param->path) - 1);
    pipe_param->iov_len = iov_len;

    pirate_set_channel_param(PIPE, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_pipe_param_t *pipe_param_get = &param_get.pipe;
    memset(pipe_param_get, 0, sizeof(*pipe_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(PIPE, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(test_path, pipe_param_get->path);
    ASSERT_EQ(iov_len, pipe_param_get->iov_len);
}

TEST(ChannelPipeTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_pipe_param_t *pipe_param = &param.pipe;
    channel_t channel;

    char default_path[128];
    snprintf(default_path, PIRATE_LEN_NAME - 1, PIRATE_PIPE_NAME_FMT, ch_num);

    char opt[128];
    const char *name = "pipe";
    const char *path = "/tmp/test_pipe";
    const uint32_t iov_len = 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(PIPE, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, pipe_param->path);
    ASSERT_EQ((unsigned)0, pipe_param->iov_len);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(PIPE, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, pipe_param->path);
    ASSERT_EQ((unsigned)0, pipe_param->iov_len);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, iov_len);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(PIPE, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, pipe_param->path);
    ASSERT_EQ(iov_len, pipe_param->iov_len);
}

class PipeTest : public ChannelTest, public WithParamInterface<int>
{
public:
    void ChannelInit()
    {
        int rv;

        rv = pirate_init_channel_param(PIPE, Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        param.pipe.iov_len = GetParam();

        rv = pirate_set_channel_param(PIPE, Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // Write and read parameters are the same
        rv = pirate_set_channel_param(PIPE, Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }
};

TEST_P(PipeTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(PipeFunctionalTest, PipeTest,
    Values(0, ChannelTest::TEST_IOV_LEN));

} // namespace
