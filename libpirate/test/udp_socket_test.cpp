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
    const char *addr1 = "1.2.3.4";
    const short port1 = 0x4242;
#ifdef _WIN32
    const char *addr2 = "0.0.0.0";
    const short port2 = 0;
#else
    const char *addr2 = "5.6.7.8";
    const short port2 = 0x4243;
#endif
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr1);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, addr1, port1);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s", name, addr1, port1, addr2);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_ERROR(EINVAL, WSAEINVAL);
    ASSERT_EQ(-1, rv);
    CROSS_PLATFORM_RESET_ERROR();

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s,%u", name, addr1, port1, addr2, port2);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr1, udp_socket_param->reader_addr);
    ASSERT_EQ(port1, udp_socket_param->reader_port);
    ASSERT_STREQ(addr2, udp_socket_param->writer_addr);
    ASSERT_EQ(port2, udp_socket_param->writer_port);
    ASSERT_EQ(0u, udp_socket_param->buffer_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s,%u,buffer_size=%u", name, addr1, port1, addr2, port2, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr1, udp_socket_param->reader_addr);
    ASSERT_EQ(port1, udp_socket_param->reader_port);
    ASSERT_STREQ(addr2, udp_socket_param->writer_addr);
    ASSERT_EQ(port2, udp_socket_param->writer_port);
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
        snprintf(param->reader_addr, sizeof(param->reader_addr) - 1, "127.0.0.1");
        snprintf(param->writer_addr, sizeof(param->writer_addr) - 1, "0.0.0.0");
        param->reader_port = 26427;
        param->writer_port = 0;
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

TEST(ChannelUdpSocketTest, WriterAddressAndPort) {
#ifndef _WIN32
    char buf[80];
    int gd_r1, gd_r2;
    int gd_w1, gd_w2;
    int rv;

    gd_r1 = pirate_open_parse("udp_socket,127.0.0.1,72600,127.0.0.1,72601", O_RDONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_r1, 0);

    gd_r2 = pirate_open_parse("udp_socket,127.0.0.1,72600,127.0.0.1,72602", O_RDONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_r2, 0);

    gd_w1 = pirate_open_parse("udp_socket,127.0.0.1,72600,127.0.0.1,72601", O_WRONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_w1, 0);

    gd_w2 = pirate_open_parse("udp_socket,127.0.0.1,72600,127.0.0.1,72602", O_WRONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_w2, 0);

    rv = pirate_write(gd_w1, "hello", 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);

    rv = pirate_write(gd_w1, "a", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w1, "b", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w1, "c", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w2, "world", 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);

    rv = pirate_read(gd_r2, buf, 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);
    // this would be "hello" if the streams were crossed
    ASSERT_STREQ("world", buf);

    rv = pirate_read(gd_r1, buf, 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);
    ASSERT_STREQ("hello", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("a", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("b", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("c", buf);

    pirate_close(gd_r1);
    pirate_close(gd_r2);
    pirate_close(gd_w1);
    pirate_close(gd_w2);
#endif
}

TEST(ChannelUdpSocketTest, WriterAddressAndPortIPv6) {
#if !defined(_WIN32) && !defined(PIRATE_LLVM_UBUNTU)
    char buf[80];
    int gd_r1, gd_r2;
    int gd_w1, gd_w2;
    int rv;

    gd_r1 = pirate_open_parse("udp_socket,::1,72600,::1,72601", O_RDONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_r1, 0);

    gd_r2 = pirate_open_parse("udp_socket,::1,72600,::1,72602", O_RDONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_r2, 0);

    gd_w1 = pirate_open_parse("udp_socket,::1,72600,::1,72601", O_WRONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_w1, 0);

    gd_w2 = pirate_open_parse("udp_socket,::1,72600,::1,72602", O_WRONLY);
    ASSERT_EQ(0, errno);
    ASSERT_GE(gd_w2, 0);

    rv = pirate_write(gd_w1, "hello", 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);

    rv = pirate_write(gd_w1, "a", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w1, "b", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w1, "c", 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);

    rv = pirate_write(gd_w2, "world", 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);

    rv = pirate_read(gd_r2, buf, 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);
    // this would be "hello" if the streams were crossed
    ASSERT_STREQ("world", buf);

    rv = pirate_read(gd_r1, buf, 6);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(6, rv);
    ASSERT_STREQ("hello", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("a", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("b", buf);

    rv = pirate_read(gd_r1, buf, 2);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(2, rv);
    ASSERT_STREQ("c", buf);

    pirate_close(gd_r1);
    pirate_close(gd_r2);
    pirate_close(gd_w1);
    pirate_close(gd_w2);
#endif
}

} // namespace
