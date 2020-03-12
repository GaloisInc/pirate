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
    const unsigned iov_len = 42;
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ("", unix_socket_param->path);
    ASSERT_EQ(0u, unix_socket_param->iov_len);
    ASSERT_EQ(0u, unix_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(0u, unix_socket_param->iov_len);
    ASSERT_EQ(0u, unix_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, iov_len);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(iov_len, unix_socket_param->iov_len);
    ASSERT_EQ(0u, unix_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, path, iov_len,
            buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UNIX_SOCKET, param.channel_type);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(iov_len, unix_socket_param->iov_len);
    ASSERT_EQ(buffer_size, unix_socket_param->buffer_size);
}

class UnixSocketTest : public ChannelTest,
    public WithParamInterface<std::tuple<int, int>>
{
public:
    void ChannelInit()
    {
        int rv;
        pirate_init_channel_param(UNIX_SOCKET, &param);
        auto test_param = GetParam();
        param.channel.unix_socket.iov_len = std::get<0>(test_param);
        param.channel.unix_socket.buffer_size = std::get<1>(test_param);

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const int TEST_BUF_LEN = 32;
};


TEST_P(UnixSocketTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(UnixSocketFunctionalTest, UnixSocketTest,
    Combine(Values(0, ChannelTest::TEST_IOV_LEN),
            Values(0, UnixSocketTest::TEST_BUF_LEN)));

} // namespace
