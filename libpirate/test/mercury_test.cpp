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

TEST(ChannelMercuryTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_MERCURY_NAME_FMT,
                channel);

    // Default configuration
    pirate_channel_param_t param;
    pirate_mercury_param_t *mercury_param = &param.mercury;
    int rv = pirate_init_channel_param(MERCURY, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, mercury_param->path);
    ASSERT_EQ((uint32_t)PIRATE_MERCURY_DEFAULT_MTU, mercury_param->mtu);

    // Apply configuration
    const char *test_path = "/tmp/test_mercury_path";
    const uint32_t mtu = PIRATE_MERCURY_DEFAULT_MTU / 2;
    strncpy(mercury_param->path, test_path, sizeof(mercury_param->path) - 1);
    mercury_param->mtu = mtu;

    rv = pirate_set_channel_param(MERCURY, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_mercury_param_t *mercury_param_get = &param_get.mercury;
    memset(mercury_param_get, 0, sizeof(*mercury_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(MERCURY, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(test_path, mercury_param_get->path);
    ASSERT_EQ(mtu, mercury_param_get->mtu);
}

TEST(ChannelMercuryTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_mercury_param_t *mercury_param = &param.mercury;
    channel_t channel;

    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_MERCURY_NAME_FMT,
                ch_num);

    char opt[128];
    const char *name = "mercury";
    const char *path = "/tmp/test_mercury";
    const uint32_t mtu = 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(MERCURY, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, mercury_param->path);
    ASSERT_EQ((uint32_t)PIRATE_MERCURY_DEFAULT_MTU, mercury_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(MERCURY, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ((uint32_t)PIRATE_MERCURY_DEFAULT_MTU, mercury_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, mtu);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(MERCURY, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ(mtu, mercury_param->mtu);
}

class MercuryTest : public ChannelTest, public WithParamInterface<uint32_t>
{
public:
    void ChannelInit()
    {
        int rv = pirate_init_channel_param(MERCURY, Writer.channel, O_WRONLY,
                                            &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        const uint32_t mtu = GetParam();
        if (mtu) {
            param.mercury.mtu = mtu;
        }

        rv = pirate_set_channel_param(MERCURY, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(MERCURY, Reader.channel, O_RDONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const uint32_t TEST_MTU = PIRATE_MERCURY_DEFAULT_MTU / 2;
};

TEST_P(MercuryTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(MercuryFunctionalTest, MercuryTest,
    Values(0, MercuryTest::TEST_MTU));

} // namespace
