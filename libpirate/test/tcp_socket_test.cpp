/* * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
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

TEST(ChannelTcpSocketTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_tcp_socket_param_t *tcp_socket_param = &param.channel.tcp_socket;

    char opt[128];
    const char *name = "tcp_socket";
    const char *addr = "1.2.3.4";
    const short port = 0x4242;
    const unsigned iov_len = 42;
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ("", tcp_socket_param->addr);
    ASSERT_EQ(0, tcp_socket_param->port);
    ASSERT_EQ(0u, tcp_socket_param->iov_len);
    ASSERT_EQ(0u, tcp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(0, tcp_socket_param->port);
    ASSERT_EQ(0u, tcp_socket_param->iov_len);
    ASSERT_EQ(0u, tcp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, addr, port);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(port, tcp_socket_param->port);
    ASSERT_EQ(0u, tcp_socket_param->iov_len);
    ASSERT_EQ(0u, tcp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, addr, port, iov_len);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(port, tcp_socket_param->port);
    ASSERT_EQ(iov_len, tcp_socket_param->iov_len);
    ASSERT_EQ(0u, tcp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u,%u", name, addr, port, iov_len,
            buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(port, tcp_socket_param->port);
    ASSERT_EQ(iov_len, tcp_socket_param->iov_len);
    ASSERT_EQ(buffer_size, tcp_socket_param->buffer_size);
}

class TcpSocketTest : public ChannelTest,
    public WithParamInterface<std::tuple<int, int>>
{
public:
    void ChannelInit()
    {
        int rv;
        pirate_init_channel_param(TCP_SOCKET, &param);
        auto test_param = GetParam();
        param.channel.tcp_socket.iov_len = std::get<0>(test_param);
        param.channel.tcp_socket.buffer_size = std::get<1>(test_param);

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const unsigned TEST_BUF_LEN = 4096;
};

TEST_P(TcpSocketTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(TcpSocketFunctionalTest, TcpSocketTest,
    Combine(Values(0, ChannelTest::TEST_IOV_LEN),
            Values(0, TcpSocketTest::TEST_BUF_LEN)));

} // namespace
