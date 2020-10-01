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
    const char *addr1 = "1.2.3.4";
    const short port1 = 0x4242;
    const char *addr2 = "5.6.7.8";
    const short port2 = 0x4243;
    const unsigned min_tx = 42;
    const unsigned buffer_size = 42 * 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr1);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, addr1, port1);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s", name, addr1, port1, addr2);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s,%u", name, addr1, port1, addr2, port2);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr1, tcp_socket_param->reader_addr);
    ASSERT_EQ(port1, tcp_socket_param->reader_port);
    ASSERT_STREQ(addr2, tcp_socket_param->writer_addr);
    ASSERT_EQ(port2, tcp_socket_param->writer_port);
    ASSERT_EQ(0u, tcp_socket_param->buffer_size);
    ASSERT_EQ(0u, tcp_socket_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s,%u,buffer_size=%u", name, addr1, port1, addr2, port2, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr1, tcp_socket_param->reader_addr);
    ASSERT_EQ(port1, tcp_socket_param->reader_port);
    ASSERT_STREQ(addr2, tcp_socket_param->writer_addr);
    ASSERT_EQ(port2, tcp_socket_param->writer_port);
    ASSERT_EQ(buffer_size, tcp_socket_param->buffer_size);
    ASSERT_EQ(0u, tcp_socket_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%s,%u,buffer_size=%u,min_tx_size=%u", name, addr1, port1, addr2, port2, buffer_size, min_tx);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(TCP_SOCKET, param.channel_type);
    ASSERT_STREQ(addr1, tcp_socket_param->reader_addr);
    ASSERT_EQ(port1, tcp_socket_param->reader_port);
    ASSERT_STREQ(addr2, tcp_socket_param->writer_addr);
    ASSERT_EQ(port2, tcp_socket_param->writer_port);
    ASSERT_EQ(buffer_size, tcp_socket_param->buffer_size);
    ASSERT_EQ(min_tx, tcp_socket_param->min_tx);
}

class TcpSocketTest : public ChannelTest,
    public WithParamInterface<std::tuple<int, int>>
{
public:
    void ChannelInit()
    {
        pirate_tcp_socket_param_t *param = &Reader.param.channel.tcp_socket;

        pirate_init_channel_param(TCP_SOCKET, &Reader.param);
        snprintf(param->reader_addr, sizeof(param->reader_addr) - 1, "127.0.0.1");
        snprintf(param->writer_addr, sizeof(param->writer_addr) - 1, "0.0.0.0");
        param->reader_port = 26427;
        param->writer_port = 0;
        auto test_param = GetParam();
        param->buffer_size = std::get<0>(test_param);
        param->min_tx = std::get<1>(test_param);
        Writer.param = Reader.param;
    }
};

static const unsigned TEST_BUF_LEN = 4096;

TEST_P(TcpSocketTest, Run)
{
    Run();
}

INSTANTIATE_TEST_SUITE_P(TcpSocketFunctionalTest, TcpSocketTest,
    Values(std::make_tuple(0, 0),
        std::make_tuple(TEST_BUF_LEN, TEST_MIN_TX_LEN)));

class TcpSocketCloseWriterTest : public ClosedWriterTest
{
public:
    void ChannelInit()
    {
        pirate_tcp_socket_param_t *param = &Reader.param.channel.tcp_socket;

        pirate_init_channel_param(TCP_SOCKET, &Reader.param);
        snprintf(param->reader_addr, sizeof(param->reader_addr) - 1, "127.0.0.1");
        snprintf(param->writer_addr, sizeof(param->writer_addr) - 1, "0.0.0.0");
        param->reader_port = 26427;
        param->writer_port = 0;
        Writer.param = Reader.param;
    }
};

TEST_F(TcpSocketCloseWriterTest, Run)
{
    Run();
}

class TcpSocketCloseReaderTest : public ClosedReaderTest
{
public:
    void ChannelInit()
    {
        pirate_tcp_socket_param_t *param = &Reader.param.channel.tcp_socket;

        pirate_init_channel_param(TCP_SOCKET, &Reader.param);
        snprintf(param->reader_addr, sizeof(param->reader_addr) - 1, "127.0.0.1");
        snprintf(param->writer_addr, sizeof(param->writer_addr) - 1, "0.0.0.0");
        param->reader_port = 26427;
        param->writer_port = 0;
        Writer.param = Reader.param;
    }
};

TEST_F(TcpSocketCloseReaderTest, Run)
{
    Run();
}

} // namespace
