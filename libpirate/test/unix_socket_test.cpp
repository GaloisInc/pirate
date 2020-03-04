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
using ::testing::Combine;

TEST(ChannelUnixSocketTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    char default_path[128];
    snprintf(default_path, PIRATE_LEN_NAME - 1, PIRATE_UNIX_SOCKET_NAME_FMT,
                channel);

    // Default configuration
    pirate_channel_param_t param;
    pirate_unix_socket_param_t *us_param = &param.unix_socket;
    int rv = pirate_init_channel_param(UNIX_SOCKET, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, us_param->path);
    ASSERT_EQ((unsigned)0, us_param->iov_len);
    ASSERT_EQ((unsigned)0, us_param->buffer_size);

    // Apply configuration
    const char *test_path = "/tmp/test_path";
    const unsigned iov_len = 42;
    const unsigned buffer_size = 1024;
    strncpy(us_param->path, test_path, sizeof(us_param->path) - 1);
    us_param->iov_len = iov_len;
    us_param->buffer_size = buffer_size;

    rv = pirate_set_channel_param(UNIX_SOCKET, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_unix_socket_param_t *us_param_get = &param_get.unix_socket;
    memset(us_param_get, 0, sizeof(*us_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(UNIX_SOCKET, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(test_path, us_param_get->path);
    ASSERT_EQ(iov_len, us_param_get->iov_len);
    ASSERT_EQ(buffer_size, us_param_get->buffer_size);
}

TEST(ChannelUnixSocketTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_unix_socket_param_t *unix_socket_param = &param.unix_socket;
    channel_t channel;

    char default_path[128];
    snprintf(default_path, PIRATE_LEN_NAME - 1, PIRATE_UNIX_SOCKET_NAME_FMT,
                ch_num);

    char opt[128];
    const char *name = "unix_socket";
    const char *path = "/tmp/test_unix_socket";
    const unsigned iov_len = 42;
    const unsigned buffer_size = 42 * 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UNIX_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, unix_socket_param->path);
    ASSERT_EQ((unsigned)0, unix_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, unix_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UNIX_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ((unsigned)0, unix_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, unix_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, iov_len);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UNIX_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, unix_socket_param->path);
    ASSERT_EQ(iov_len, unix_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, unix_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, path, iov_len,
            buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UNIX_SOCKET, channel);
    ASSERT_EQ(0, errno);
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
        int rv = pirate_init_channel_param(UNIX_SOCKET, Writer.channel,
                                            O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        auto test_param = GetParam();
        param.unix_socket.iov_len = std::get<0>(test_param);
        param.unix_socket.buffer_size = std::get<1>(test_param);

        rv = pirate_set_channel_param(UNIX_SOCKET, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(UNIX_SOCKET, Reader.channel, O_RDONLY, &param);
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
