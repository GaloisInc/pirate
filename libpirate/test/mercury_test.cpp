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

#include <cstring>
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;


TEST(ChannelMercuryTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_mercury_param_t *mercury_param = &param.mercury;

    char opt[128];
    const char *name = "mercury";
    const uint32_t application_id = 0x42;
    const char *path = "/tmp/test_mercury";
    const uint32_t mtu = 42;
    const uint32_t timeout_ms = 2000;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_EQ(0u, mercury_param->application_id);
    ASSERT_STREQ("", mercury_param->path);
    ASSERT_EQ(0u, mercury_param->mtu);
    ASSERT_EQ(0u, mercury_param->timeout_ms);

    snprintf(opt, sizeof(opt) - 1, "%s,%d", name, application_id);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_EQ(application_id, mercury_param->application_id);
    ASSERT_STREQ("", mercury_param->path);
    ASSERT_EQ(0u, mercury_param->mtu);
    ASSERT_EQ(0u, mercury_param->timeout_ms);

    snprintf(opt, sizeof(opt) - 1, "%s,%d,%s", name, application_id, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_EQ(application_id, mercury_param->application_id);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ(0u, mercury_param->mtu);
    ASSERT_EQ(0u, mercury_param->timeout_ms);

    snprintf(opt, sizeof(opt) - 1, "%s,%d,%s,%u", name, application_id, path,
            mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_EQ(application_id, mercury_param->application_id);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ(mtu, mercury_param->mtu);
    ASSERT_EQ(0u, mercury_param->timeout_ms);

    snprintf(opt, sizeof(opt) - 1, "%s,%d,%s,%u,%u", name, application_id, path,
            mtu, timeout_ms);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_EQ(application_id, mercury_param->application_id);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ(mtu, mercury_param->mtu);
    ASSERT_EQ(timeout_ms, mercury_param->timeout_ms);
}

TEST(ChannelMercuryTest, BasicFunctionality) {
    int rv = 0;
    const int channel = 0;

    const uint8_t wr_data[] = { 0xC0, 0xDE, 0xDA, 0xDA };
    const ssize_t data_len = sizeof(wr_data);
    uint8_t rd_data[data_len] = { 0 };
    ssize_t io_size = -1;

    if (access(PIRATE_MERCURY_ROOT_DEV, R_OK | W_OK) != 0) {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
        return;
    }

    pirate_channel_param_t param;
    pirate_init_channel_param(MERCURY, &param);

    rv =  pirate_set_channel_param(channel, O_WRONLY, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv =  pirate_set_channel_param(channel, O_RDONLY, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_open(channel, O_WRONLY);
    ASSERT_EQ(channel, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_open(channel, O_RDONLY);
    ASSERT_EQ(channel, rv);
    ASSERT_EQ(0, errno);

    io_size = pirate_write(channel, wr_data, data_len);
    ASSERT_EQ(io_size, data_len);
    ASSERT_EQ(0, errno);

    io_size = pirate_read(channel, rd_data, data_len);
    ASSERT_EQ(io_size, data_len);
    ASSERT_EQ(0, errno);

    EXPECT_TRUE(0 == std::memcmp(wr_data, rd_data, data_len));

    rv = pirate_close(channel, O_WRONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_close(channel, O_RDONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
}

class MercuryTest : public ChannelTest, public WithParamInterface<int>
{
public:
    void ChannelInit()
    {
        int rv;

        Writer.channel = Reader.channel = GetParam();

        // Writer
        pirate_init_channel_param(MERCURY, &param);
        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // Rreader
        pirate_init_channel_param(MERCURY, &param);
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const uint32_t TEST_MTU = PIRATE_MERCURY_DEFAULT_MTU / 2;
};

TEST_P(MercuryTest, Run)
{
    if (access(PIRATE_MERCURY_ROOT_DEV, R_OK | W_OK) == 0) {
        Run();
    } else {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
    }
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(MercuryFunctionalTest, MercuryTest,
    Values(0));

} // namespace
