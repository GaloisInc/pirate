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

#ifdef _WIN32
#include "windows/libpirate.h"
#else
#include "libpirate.h"
#endif
#include "channel_test.hpp"
#include "cross_platform_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;

TEST(ChannelUdpSocketTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_udp_socket_param_t *udp_socket_param = &param.channel.udp_socket;

    char opt[128];
    const char *name = "udp_socket";
    const char *addr = "1.2.3.4";
    const short port = 0x4242;
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, addr, port);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, udp_socket_param->addr);
    ASSERT_EQ(port, udp_socket_param->port);
    ASSERT_EQ(0u, udp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,buffer_size=%u", name, addr, port, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr, udp_socket_param->addr);
    ASSERT_EQ(port, udp_socket_param->port);
    ASSERT_EQ(buffer_size, udp_socket_param->buffer_size);
}

class UdpSocketTest : public ChannelTest,
    public WithParamInterface<int>
{
public:
    void ChannelInit()
    {
        pirate_udp_socket_param_t *param = &Reader.param.channel.udp_socket;

        pirate_init_channel_param(UDP_SOCKET, &Reader.param);
        snprintf(param->addr, sizeof(param->addr) - 1, PIRATE_DEFAULT_UDP_IP_ADDR);
        param->port = 26427;
        param->buffer_size = GetParam();
        Writer.param = Reader.param;
    }

    void TearDown()
    {
        ChannelTest::TearDown();
        ASSERT_EQ(1, nonblocking_IO_attempt);
    }

    static const int TEST_BUF_LEN = 4096;
};

TEST_P(UdpSocketTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(UdpSocketFunctionalTest, UdpSocketTest,
    Values(0, UdpSocketTest::TEST_BUF_LEN));

} // namespace
