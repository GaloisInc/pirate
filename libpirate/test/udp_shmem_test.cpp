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

TEST(ChannelUdpShmemTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    // Default configuration
    pirate_channel_param_t param;

    int rv = pirate_init_channel_param(UDP_SHMEM, channel, flags, &param);
#if PIRATE_SHMEM_FEATURE
    pirate_udp_shmem_param_t *udp_shmem_param = &param.udp_shmem;
    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SHMEM_NAME, channel);

    ASSERT_STREQ(default_path, udp_shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, udp_shmem_param->buffer_size);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_COUNT, udp_shmem_param->packet_count);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_SIZE, udp_shmem_param->packet_size);

    // Apply configuration
    const char *path = "/tmp/gaps_udp_shmem_test";
    const unsigned buffer_size = DEFAULT_SMEM_BUF_LEN * 2;
    const unsigned packet_count = DEFAULT_UDP_SHMEM_PACKET_COUNT * 2;
    const unsigned packet_size = DEFAULT_UDP_SHMEM_PACKET_SIZE * 2;

    strncpy(udp_shmem_param->path, path, sizeof(udp_shmem_param->path) - 1);
    udp_shmem_param->buffer_size = buffer_size;
    udp_shmem_param->packet_count = packet_count;
    udp_shmem_param->packet_size = packet_size;

    rv = pirate_set_channel_param(UDP_SHMEM, channel, flags, &param);

    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_udp_shmem_param_t *udp_shmem_param_get = &param_get.udp_shmem;
    memset(udp_shmem_param_get, 0, sizeof(*udp_shmem_param_get));

    channel_t ch =  pirate_get_channel_param(channel, O_RDONLY, &param_get);
    ASSERT_EQ(UDP_SHMEM, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, udp_shmem_param_get->path);
    ASSERT_EQ(buffer_size, udp_shmem_param_get->buffer_size);
    ASSERT_EQ(packet_count, udp_shmem_param_get->packet_count);
    ASSERT_EQ(packet_size, udp_shmem_param_get->packet_size);
#else
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(ESOCKTNOSUPPORT, errno);
    errno = 0;
#endif
}

TEST(ChannelUdpShmemTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    channel_t channel;

    char opt[128];
    const char *name = "udp_shmem";
    const char *path = "/tmp/test_udp_shmem";
    const unsigned buffer_size = 42 * 42;

#if PIRATE_SHMEM_FEATURE
    const pirate_udp_shmem_param_t *udp_shmem_param = &param.udp_shmem;
    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SHMEM_NAME, ch_num);
    const unsigned packet_size = 4242;
    const unsigned packet_count = 4224;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, udp_shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, udp_shmem_param->buffer_size);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_COUNT, udp_shmem_param->packet_count);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_SIZE, udp_shmem_param->packet_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ((unsigned)DEFAULT_SMEM_BUF_LEN, udp_shmem_param->buffer_size);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_COUNT, udp_shmem_param->packet_count);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_SIZE, udp_shmem_param->packet_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UDP_SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_COUNT, udp_shmem_param->packet_count);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_SIZE, udp_shmem_param->packet_size);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, path, buffer_size,
                packet_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UDP_SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(packet_size, udp_shmem_param->packet_size);
    ASSERT_EQ(DEFAULT_UDP_SHMEM_PACKET_COUNT, udp_shmem_param->packet_count);


    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u,%u", name, path, buffer_size,
                packet_size, packet_count);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UDP_SHMEM, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(packet_size, udp_shmem_param->packet_size);
    ASSERT_EQ(packet_count, udp_shmem_param->packet_count);
#else
    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(INVALID, channel);
    ASSERT_EQ(ESOCKTNOSUPPORT, errno);
    errno = 0;
#endif
}


#if PIRATE_SHMEM_FEATURE
class UdpShmemTest : public ChannelTest,
    public WithParamInterface<std::tuple<int, int, int>>
{
public:
    void ChannelInit()
    {
        int rv = pirate_init_channel_param(UDP_SHMEM, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        auto test_param = GetParam();
        unsigned buffer_size = std::get<0>(test_param);
        unsigned packet_count = std::get<1>(test_param);
        unsigned packet_size = std::get<2>(test_param);

        if (buffer_size) {
            param.udp_shmem.buffer_size = buffer_size;
        }

        if (packet_count) {
            param.udp_shmem.packet_count = packet_count;
        }

        if (packet_size) {
            param.udp_shmem.packet_size = packet_size;
        }

        rv = pirate_set_channel_param(UDP_SHMEM, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(UDP_SHMEM, Reader.channel, O_RDONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const int TEST_BUF_LEN = DEFAULT_SMEM_BUF_LEN / 2;
    static const int TEST_PKT_CNT = DEFAULT_UDP_SHMEM_PACKET_COUNT / 2;
    static const int TEST_PKT_SIZE = DEFAULT_UDP_SHMEM_PACKET_SIZE / 2;
};

TEST_P(UdpShmemTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(ShmemFunctionalTest, UdpShmemTest,
    Combine(Values(0, UdpShmemTest::TEST_BUF_LEN),
            Values(0, UdpShmemTest::TEST_PKT_CNT),
            Values(0, UdpShmemTest::TEST_PKT_SIZE)));
#endif

} // namespace
