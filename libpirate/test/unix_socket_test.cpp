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
using ::testing::Combine;

TEST(ChannelUnixSocketTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_unix_socket_param_t *unix_socket_param = &param.channel.unix_socket;

    char opt[128];
    const char *name = "unix_socket";
    const char *path = "/tmp/test_unix_socket";
    const unsigned min_tx = 42;
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(0u, unix_socket_param->buffer_size);
    ASSERT_EQ(0u, unix_socket_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,buffer_size=%u", name, path, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(buffer_size, unix_socket_param->buffer_size);
    ASSERT_EQ(0u, unix_socket_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,buffer_size=%u,min_tx_size=%u", name, path, buffer_size, min_tx);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(buffer_size, unix_socket_param->buffer_size);
    ASSERT_EQ(min_tx, unix_socket_param->min_tx);
}

class UnixSocketTest : public ChannelTest,
    public WithParamInterface<std::tuple<int, int>>
{
public:
    void ChannelInit()
    {
        pirate_unix_socket_param_t *param = &Reader.param.channel.unix_socket;

        pirate_init_channel_param(UNIX_SOCKET, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test.sock", PIRATE_LEN_NAME);
        auto test_param = GetParam();
        param->buffer_size = std::get<0>(test_param);
        param->min_tx = std::get<1>(test_param);
        Writer.param = Reader.param;
    }
};

static const int TEST_BUF_LEN = 32;

TEST_P(UnixSocketTest, Run)
{
    Run();
}

INSTANTIATE_TEST_SUITE_P(UnixSocketFunctionalTest, UnixSocketTest,
    Values(std::make_tuple(0, 0),
        std::make_tuple(TEST_BUF_LEN, TEST_MIN_TX_LEN)));

class UnixSocketCloseWriterTest : public ClosedWriterTest
{
public:
    void ChannelInit()
    {
        pirate_unix_socket_param_t *param = &Reader.param.channel.unix_socket;

        pirate_init_channel_param(UNIX_SOCKET, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test.sock", PIRATE_LEN_NAME);
        Writer.param = Reader.param;
    }
};

TEST_F(UnixSocketCloseWriterTest, Run)
{
    Run();
}

class UnixSocketCloseReaderTest : public ClosedReaderTest
{
public:
    void ChannelInit()
    {
        pirate_unix_socket_param_t *param = &Reader.param.channel.unix_socket;

        pirate_init_channel_param(UNIX_SOCKET, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test.sock", PIRATE_LEN_NAME);
        Writer.param = Reader.param;
    }
};

TEST_F(UnixSocketCloseReaderTest, Run)
{
    Run();
}

} // namespace
