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


TEST(ChannelUdpShmemTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;

    char opt[128];
    const char *name = "udp_shmem";
    const char *path = "/tmp/test_udp_shmem";
    const unsigned buffer_size = 42 * 42;

#if PIRATE_SHMEM_FEATURE
    const pirate_udp_shmem_param_t *udp_shmem_param = &param.channel.udp_shmem;
    const unsigned packet_size = 4242;
    const unsigned packet_count = 4224;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SHMEM, param.channel_type);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(0u, udp_shmem_param->buffer_size);
    ASSERT_EQ(0u, udp_shmem_param->packet_count);
    ASSERT_EQ(0u, udp_shmem_param->packet_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SHMEM, param.channel_type);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(0u, udp_shmem_param->packet_count);
    ASSERT_EQ(0u, udp_shmem_param->packet_size);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u", name, path, buffer_size,
                packet_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UDP_SHMEM, param.channel_type);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(packet_size, udp_shmem_param->packet_size);
    ASSERT_EQ(0u, udp_shmem_param->packet_count);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u,%u,%u", name, path, buffer_size,
                packet_size, packet_count);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(UDP_SHMEM, param.channel_type);
    ASSERT_STREQ(path, udp_shmem_param->path);
    ASSERT_EQ(buffer_size, udp_shmem_param->buffer_size);
    ASSERT_EQ(packet_size, udp_shmem_param->packet_size);
    ASSERT_EQ(packet_count, udp_shmem_param->packet_count);
#else
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, buffer_size);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(-1, rv);
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
        char opt[128];
        pirate_udp_shmem_param_t *param = &Reader.param.channel.udp_shmem;

        const char *testPath = "/gaps.shmem_test";
        pirate_init_channel_param(UDP_SHMEM, &Reader.param);
        strncpy(param->path, testPath, PIRATE_LEN_NAME - 1);
        auto test_param = GetParam();
        unsigned buffer_size = std::get<0>(test_param);
        unsigned packet_count = std::get<1>(test_param);
        unsigned packet_size = std::get<2>(test_param);

        if (buffer_size) {
            param->buffer_size = buffer_size;
        } else {
            buffer_size = DEFAULT_SMEM_BUF_LEN;
        }

        if (packet_count) {
            param->packet_count = packet_count;
        } else {
            packet_count = DEFAULT_UDP_SHMEM_PACKET_COUNT;
        }

        if (packet_size) {
            param->packet_size = packet_size;
        } else {
            packet_size = DEFAULT_UDP_SHMEM_PACKET_SIZE;
        }

        Writer.param = Reader.param;

        snprintf(opt, sizeof(opt) - 1, "udp_shmem,%s,%u,%u,%u",
                    param->path, buffer_size, packet_size,
                    packet_count);
        Reader.desc.assign(opt);
        Writer.desc.assign(opt);
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
