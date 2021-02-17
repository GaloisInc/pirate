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
using ::testing::ValuesIn;

TEST(ChannelMercuryTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t expParam, rdParam;
    char opt[256];
    const char *name = "mercury";
    const char *mode = "immediate";
    uint32_t session_id = 1;
    uint32_t message_id = 2;
    uint32_t data_tag = 3;
    uint32_t descriptor_tag = 4;
    uint32_t mtu = 5;

    pirate_init_channel_param(MERCURY, &expParam);
    pirate_init_channel_param(MERCURY, &rdParam);

    expParam.channel.mercury.mode = MERCURY_IMMEDIATE;
    expParam.channel.mercury.session_id = session_id;
    expParam.channel.mercury.message_id = message_id;
    expParam.channel.mercury.data_tag = data_tag;
    expParam.channel.mercury.descriptor_tag = descriptor_tag;
    expParam.channel.mercury.mtu = mtu;

    snprintf(opt, sizeof(opt) - 1, "%s,mode=%s,session=%u,message=%u,data=%u,descriptor=%u,mtu=%u",
        name, mode, session_id, message_id, data_tag, descriptor_tag, mtu);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));
}

TEST(ChannelMercuryTest, UnparseChannelParam)
{
    char *output;
    int rv;
    pirate_channel_param_t param;

    rv = pirate_parse_channel_param("mercury,mode=immediate,mtu=5,descriptor=4,data=3,message=2,session=1", &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    output = (char*) calloc(69, sizeof(char));
    rv = pirate_unparse_channel_param(&param, output, 69);
    ASSERT_EQ(68, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ("mercury,mode=immediate,session=1,message=2,data=3,descriptor=4,mtu=5", output);
    free(output);
}

class MercuryTest : public ChannelTest, public WithParamInterface<pirate_mercury_param_t>
{
public:
    void ChannelInit() override
    {
        mMercuryParam = GetParam();
        Writer.param.channel_type = MERCURY;
        Writer.param.channel.mercury = mMercuryParam;
        Reader.param = Writer.param;
    }

protected:
    pirate_mercury_param_t mMercuryParam;
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

#define IMM MERCURY_IMMEDIATE
#define PAY MERCURY_PAYLOAD

static pirate_mercury_param_t MercuryParams [] =
{
// these are the values that work
// MODE, SESS, MSG, DATA, DESC, MTU
{  IMM,    1,    1,   1,    0,    0},
{  IMM,    1,    1,   3,    0,    0},
{  IMM,    1,    2,   3,    0,    0},
{  IMM,    1,    3,   3,    0,    0},
{  IMM,    2,    2,   1,    0,    0},
{  IMM,    2,    2,   2,    0,    0},
{  IMM,    2,    3,   3,    0,    0},

{  PAY,    1,    1,   1,    1,    0},
{  PAY,    1,    1,   3,    3,    0},
{  PAY,    1,    2,   3,    3,    0},
{  PAY,    1,    3,   3,    3,    0},
{  PAY,    2,    2,   1,    1,    0},
{  PAY,    2,    2,   2,    2,    0},
{  PAY,    2,    3,   3,    3,    0},
};

INSTANTIATE_TEST_SUITE_P(MercuryFunctionalTest, MercuryTest,
    ValuesIn(MercuryParams));

} // namespace
