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


TEST(ChannelMercuryTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_mercury_param_t *mercury_param = &param.mercury;

    char opt[128];
    const char *name = "mercury";
    const char *path = "/tmp/test_mercury";
    const uint32_t mtu = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_STREQ("", mercury_param->path);
    ASSERT_EQ((unsigned)0, mercury_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ((unsigned)0, mercury_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(MERCURY, param.channel_type);
    ASSERT_STREQ(path, mercury_param->path);
    ASSERT_EQ(mtu, mercury_param->mtu);
}

class MercuryTest : public ChannelTest, public WithParamInterface<uint32_t>
{
public:
    void ChannelInit()
    {
        int rv;
        pirate_init_channel_param(MERCURY, &param);
        const uint32_t mtu = GetParam();
        if (mtu) {
            param.mercury.mtu = mtu;
        }

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
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
