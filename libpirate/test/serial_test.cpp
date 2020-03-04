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

TEST(ChannelSerialTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SERIAL_NAME_FMT,
                channel);

    // Default configuration
    pirate_channel_param_t param;
    pirate_serial_param_t *serial_param = &param.serial;
    int rv = pirate_init_channel_param(SERIAL, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, serial_param->path);
    ASSERT_EQ((speed_t)SERIAL_DEFAULT_BAUD, serial_param->baud);
    ASSERT_EQ((unsigned)SERIAL_DEFAULT_MTU, serial_param->mtu);

    // Apply configuration
    const char *test_dev = "/dev/ttyUSB2";
    const speed_t baud = B115200;
    const unsigned mtu = SERIAL_DEFAULT_MTU / 2;
    strncpy(serial_param->path, test_dev, sizeof(serial_param->path) - 1);
    serial_param->baud = baud;
    serial_param->mtu = mtu;

    rv = pirate_set_channel_param(SERIAL, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_serial_param_t *serial_param_get = &param_get.serial;
    memset(serial_param_get, 0, sizeof(*serial_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(SERIAL, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(test_dev, serial_param_get->path);
    ASSERT_EQ(baud, serial_param_get->baud);
    ASSERT_EQ(mtu, serial_param_get->mtu);
}

TEST(ChannelSerialTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_serial_param_t *serial_param = &param.serial;
    channel_t channel;

    char default_path[128];
    snprintf(default_path, sizeof(default_path), PIRATE_SERIAL_NAME_FMT,
                ch_num);

    char opt[128];
    const char *name = "serial";
    const char *path = "/tmp/test_serial";
    const speed_t baud = B115200;
    const char *baud_str = "115200";
    const char *invalid_baud_str = "115201";
    const uint32_t mtu = 42;

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SERIAL, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(default_path, serial_param->path);
    ASSERT_EQ((speed_t)SERIAL_DEFAULT_BAUD, serial_param->baud);
    ASSERT_EQ((unsigned)SERIAL_DEFAULT_MTU, serial_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SERIAL, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ((speed_t)SERIAL_DEFAULT_BAUD, serial_param->baud);
    ASSERT_EQ((unsigned)SERIAL_DEFAULT_MTU, serial_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s", name, path, baud_str);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SERIAL, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ((unsigned)SERIAL_DEFAULT_MTU, serial_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s,%u", name, path, baud_str, mtu);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(SERIAL, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ(mtu, serial_param->mtu);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s,%s,%u", name, path,
                invalid_baud_str, mtu);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(INVALID, channel);
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

        int rv = pirate_init_channel_param(SERIAL, Writer.channel, O_WRONLY,
                                            &wr_param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
        if (baud) {
            wr_param.serial.baud = baud;
        }

        if (mtu) {
            wr_param.serial.mtu = mtu;
        }

        rv = pirate_set_channel_param(SERIAL, Writer.channel, O_WRONLY,
                                        &wr_param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        pirate_init_channel_param(SERIAL, Reader.channel, O_WRONLY, &rd_param);
        if (baud) {
            rd_param.serial.baud = baud;
        }

        if (mtu) {
            rd_param.serial.mtu = mtu;
        }

        rv = pirate_set_channel_param(SERIAL, Reader.channel, O_RDONLY,
                                        &rd_param);
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
