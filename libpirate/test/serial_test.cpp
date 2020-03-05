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

TEST(ChannelSerialTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_serial_param_t *serial_param = &param.serial;

    char opt[128];
    const char *name = "serial";
    const char *path = "/tmp/test_serial";
    const speed_t baud = B115200;
    const char *baud_str = "115200";
    const char *invalid_baud_str = "115201";
    const uint32_t mtu = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ("", serial_param->path);
    ASSERT_EQ((speed_t)0, serial_param->baud);
    ASSERT_EQ((unsigned)0, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ((speed_t)0, serial_param->baud);
    ASSERT_EQ((unsigned)0, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s", name, path, baud_str);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ((unsigned)0, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s,%u", name, path, baud_str, mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ(mtu, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s,%u", name, path,
                invalid_baud_str, mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;
}

class SerialTest : public ChannelTest,
    public WithParamInterface<std::tuple<speed_t, int>>
{
public:
    void ChannelInit()
    {
        Writer.channel = 0;
        Reader.channel = 1;

        auto test_param = GetParam();
        const speed_t baud = std::get<0>(test_param);
        const unsigned mtu = std::get<0>(test_param);
        int rv;

        pirate_init_channel_param(SERIAL, &wr_param);
        if (baud) {
            wr_param.serial.baud = baud;
        }

        if (mtu) {
            wr_param.serial.mtu = mtu;
        }

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &wr_param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        pirate_init_channel_param(SERIAL, &rd_param);
        if (baud) {
            rd_param.serial.baud = baud;
        }

        if (mtu) {
            rd_param.serial.mtu = mtu;
        }

        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &rd_param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    static const speed_t TEST_BAUD = B115200;
    static const unsigned TEST_MTU = SERIAL_DEFAULT_MTU / 2;

    pirate_channel_param_t wr_param;
    pirate_channel_param_t rd_param;
};


TEST_P(SerialTest, Run)
{
    if ((access("/dev/ttyUSB0", W_OK) == 0) &&
        (access("/dev/ttyUSB1", R_OK) == 0)) {
        Run();
    }
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(SerialFunctionalTest, SerialTest,
    Combine(Values(0, SerialTest::TEST_BAUD),
            Values(0, SerialTest::TEST_MTU)));

} // namespace
