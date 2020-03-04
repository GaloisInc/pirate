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

#include "primitives.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

TEST(ChannelTcpSocketTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL + 1;
    const int flags = O_RDONLY;

    // Default configuration
    pirate_channel_param_t param;
    pirate_tcp_socket_param_t *tcp_param = &param.tcp_socket;
    int rv = pirate_init_channel_param(TCP_SOCKET, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_TCP_IP_ADDR, tcp_param->addr);
    ASSERT_EQ(PIRATE_TCP_PORT_BASE + channel, tcp_param->port);
    ASSERT_EQ((unsigned)0, tcp_param->iov_len);
    ASSERT_EQ((unsigned)0, tcp_param->buffer_size);

    // Apply configuration
    const char *ip_addr = "1.2.3.4";
    const short ip_port = 4321;
    const unsigned iov_len = 42;
    const unsigned buffer_size = 128;

    strncpy(tcp_param->addr, ip_addr, sizeof(tcp_param->addr) - 1);
    tcp_param->port = ip_port;
    tcp_param->iov_len = iov_len;
    tcp_param->buffer_size = buffer_size;

    rv = pirate_set_channel_param(TCP_SOCKET, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_tcp_socket_param_t *tcp_param_get = &param_get.tcp_socket;
    memset(tcp_param_get, 0, sizeof(*tcp_param_get));

    channel_t ch =  pirate_get_channel_param(channel, O_RDONLY, &param_get);
    ASSERT_EQ(TCP_SOCKET, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(ip_addr, tcp_param_get->addr);
    ASSERT_EQ(ip_port, tcp_param_get->port);
    ASSERT_EQ(iov_len, tcp_param_get->iov_len);
    ASSERT_EQ(buffer_size, tcp_param_get->buffer_size);
}

TEST(ChannelTcpSocketTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_tcp_socket_param_t *tcp_socket_param = &param.tcp_socket;
    channel_t channel;

    char opt[128];
    const char *name = "tcp_socket";
    const char *addr = "1.2.3.4";
    const short port = 0x4242;
    const unsigned iov_len = 42;
    const unsigned buffer_size = 42 * 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(TCP_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_TCP_IP_ADDR, tcp_socket_param->addr);
    ASSERT_EQ(PIRATE_TCP_PORT_BASE + ch_num, tcp_socket_param->port);
    ASSERT_EQ((unsigned)0, tcp_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, tcp_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(TCP_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(PIRATE_TCP_PORT_BASE + ch_num, tcp_socket_param->port);
    ASSERT_EQ((unsigned)0, tcp_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, tcp_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, addr, port);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(TCP_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(port, tcp_socket_param->port);
    ASSERT_EQ((unsigned)0, tcp_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, tcp_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, addr, port, iov_len);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(TCP_SOCKET, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, tcp_socket_param->addr);
    ASSERT_EQ(port, tcp_socket_param->port);
    ASSERT_EQ(iov_len, tcp_socket_param->iov_len);
    ASSERT_EQ((unsigned)0, tcp_socket_param->buffer_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u,%u", name, addr, port, iov_len,
            buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(TCP_SOCKET, channel);
    ASSERT_EQ(0, errno);
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
        int rv = pirate_init_channel_param(TCP_SOCKET, Writer.channel, O_WRONLY,
                                            &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        auto test_param = GetParam();
        param.tcp_socket.iov_len = std::get<0>(test_param);
        param.tcp_socket.buffer_size = std::get<1>(test_param);

        rv = pirate_set_channel_param(TCP_SOCKET, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(TCP_SOCKET, Reader.channel, O_RDONLY, &param);
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
