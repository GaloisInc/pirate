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

TEST(ChannelGeEthTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL + 1;
    const int flags = O_RDONLY;

    // Default configuration
    pirate_channel_param_t param;
    pirate_ge_eth_param_t *ge_eth_param = &param.ge_eth;
    int rv = pirate_init_channel_param(GE_ETH, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_GE_ETH_IP_ADDR, ge_eth_param->addr);
    ASSERT_EQ(DEFAULT_GE_ETH_IP_PORT + channel, ge_eth_param->port);
    ASSERT_EQ((unsigned)DEFAULT_GE_ETH_MTU, ge_eth_param->mtu);

    // Apply configuration
    const char *ip_addr = "1.2.3.4";
    const short ip_port = 0x4321;
    const unsigned mtu = DEFAULT_GE_ETH_MTU / 2;

    strncpy(ge_eth_param->addr, ip_addr, sizeof(ge_eth_param->addr) - 1);
    ge_eth_param->port = ip_port;
    ge_eth_param->mtu = mtu;

    rv = pirate_set_channel_param(GE_ETH, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_ge_eth_param_t *ge_eth_param_get = &param_get.ge_eth;
    memset(ge_eth_param_get, 0, sizeof(*ge_eth_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(GE_ETH, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(ip_addr, ge_eth_param_get->addr);
    ASSERT_EQ(ip_port, ge_eth_param_get->port);
    ASSERT_EQ(mtu, ge_eth_param_get->mtu);
}

TEST(ChannelGeEthTest, ConfigurationParser) {

    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_ge_eth_param_t *ge_eth_param = &param.ge_eth;
    channel_t channel;

    char opt[128];
    const char *name = "ge_eth";
    const char *addr = "1.2.3.4";
    const short port = 0x4242;
    const unsigned mtu = 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(GE_ETH, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_GE_ETH_IP_ADDR, ge_eth_param->addr);
    ASSERT_EQ(DEFAULT_GE_ETH_IP_PORT + ch_num, ge_eth_param->port);
    ASSERT_EQ((unsigned)DEFAULT_GE_ETH_MTU, ge_eth_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, addr);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(GE_ETH, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(DEFAULT_GE_ETH_IP_PORT + ch_num, ge_eth_param->port);
    ASSERT_EQ((unsigned)DEFAULT_GE_ETH_MTU, ge_eth_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%d", name, addr, port);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(GE_ETH, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(port, ge_eth_param->port);
    ASSERT_EQ((unsigned)DEFAULT_GE_ETH_MTU, ge_eth_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%d,%u", name, addr, port, mtu);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(GE_ETH, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(addr, ge_eth_param->addr);
    ASSERT_EQ(port, ge_eth_param->port);
    ASSERT_EQ(mtu, ge_eth_param->mtu);
}

class GeEthTest : public ChannelTest, public WithParamInterface<unsigned>
{
public:
    void ChannelInit()
    {
        // Since UDP is unreliable it's possible to get out of sync.
        // Use write delay to reduce the chance of that
        WriteDelayUs = 1000;

        int rv = pirate_init_channel_param(GE_ETH, Writer.channel, O_WRONLY,
                                            &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        const unsigned mtu = GetParam();
        if (mtu) {
            param.ge_eth.mtu = mtu;
        }

        rv = pirate_set_channel_param(GE_ETH, Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(GE_ETH, Reader.channel, O_RDONLY, &param);
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
