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
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;


TEST(ChannelPipeTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_pipe_param_t *pipe_param = &param.channel.pipe;

    char opt[128];
    const char *name = "pipe";
    const char *path = "/tmp/test_pipe";
    const unsigned min_tx = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(PIPE, param.channel_type);
    ASSERT_STREQ(path, pipe_param->path);
    ASSERT_EQ(0u, pipe_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,min_tx_size=%u", name, path, min_tx);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(PIPE, param.channel_type);
    ASSERT_STREQ(path, pipe_param->path);
    ASSERT_EQ(min_tx, pipe_param->min_tx);
}

class PipeTest : public ChannelTest, public WithParamInterface<std::tuple<int, int>>
{
public:
    void ChannelInit()
    {
        pirate_pipe_param_t *param = &Reader.param.channel.pipe;

        pirate_init_channel_param(PIPE, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test", PIRATE_LEN_NAME);
        auto params = GetParam();
        param->mtu = std::get<0>(params);
        param->min_tx = std::get<1>(params);
        Writer.param = Reader.param;
    }

    void TearDown()
    {
        pirate_pipe_param_t *param = &Reader.param.channel.pipe;
        unsigned int mtu = param->mtu;
        ChannelTest::TearDown();
        if (mtu > 0) {
            ASSERT_EQ(1, nonblocking_IO_attempt);
        } else {
            ASSERT_EQ(0, nonblocking_IO_attempt);
        }
    }
};

TEST_P(PipeTest, Run)
{
    Run();
}

INSTANTIATE_TEST_SUITE_P(PipeFunctionalTest, PipeTest,
    Combine(Values(0, 512), Values(0, TEST_MIN_TX_LEN)));

class PipeCloseWriterTest : public ClosedWriterTest
{
public:
    void ChannelInit()
    {
        pirate_pipe_param_t *param = &Reader.param.channel.pipe;

        pirate_init_channel_param(PIPE, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test", PIRATE_LEN_NAME);
        Writer.param = Reader.param;
    }
};

TEST_F(PipeCloseWriterTest, Run)
{
    Run();
}

class PipeCloseReaderTest : public ClosedReaderTest
{
public:
    void ChannelInit()
    {
        pirate_pipe_param_t *param = &Reader.param.channel.pipe;

        pirate_init_channel_param(PIPE, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test", PIRATE_LEN_NAME);
        Writer.param = Reader.param;
    }
};

TEST_F(PipeCloseReaderTest, Run)
{
    Run();
}

} // namespace
