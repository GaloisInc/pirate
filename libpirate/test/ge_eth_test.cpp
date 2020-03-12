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

TEST(ChannelGeEthTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_ge_eth_param_t *ge_eth_param = &param.ge_eth;

    char opt[128];
    const char *name = "ge_eth";
    const char *addr = "1.2.3.4";
    const short port = 0x4242;
    const unsigned mtu = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(GE_ETH, param.channel_type);
    ASSERT_STREQ("", ge_eth_param->addr);
    ASSERT_EQ(0, ge_eth_param->port);
    ASSERT_EQ(0u, ge_eth_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(GE_ETH, param.channel_type);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(0, ge_eth_param->port);
    ASSERT_EQ(0u, ge_eth_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%d", name, addr, port);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(GE_ETH, param.channel_type);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(port, ge_eth_param->port);
    ASSERT_EQ(0u, ge_eth_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%d,%u", name, addr, port, mtu);
    rv  = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(GE_ETH, param.channel_type);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(port, ge_eth_param->port);
    ASSERT_EQ(mtu, ge_eth_param->mtu);
}

class GeEthTest : public ChannelTest, public WithParamInterface<unsigned>
{
public:
    void ChannelInit()
    {
        int rv;

        // Since UDP is unreliable it's possible to get out of sync.
        // Use write delay to reduce the chance of that
        WriteDelayUs = 1000;

        pirate_init_channel_param(GE_ETH, &param);
        const unsigned mtu = GetParam();
        if (mtu) {
            param.ge_eth.mtu = mtu;
        }

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const unsigned TEST_MTU_LEN = DEFAULT_GE_ETH_MTU / 2;
};


TEST_P(GeEthTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(GeEthFunctionalTest, GeEthTest,
                        Values(0, GeEthTest::TEST_MTU_LEN));

} // namespace
