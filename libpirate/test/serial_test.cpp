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
    const pirate_serial_param_t *serial_param = &param.channel.serial;

    char opt[128];
    const char *name = "serial";
    const char *path = "/tmp/test_serial";
    const speed_t baud = B115200;
    const char *baud_str = "115200";
    const char *invalid_baud_str = "115201";
    const uint32_t mtu = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ((speed_t)0, serial_param->baud);
    ASSERT_EQ(0u, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,baud=%s", name, path, baud_str);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ(0u, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,baud=%s,mtu=%u", name, path, baud_str, mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(SERIAL, param.channel_type);
    ASSERT_STREQ(path, serial_param->path);
    ASSERT_EQ(baud, serial_param->baud);
    ASSERT_EQ(mtu, serial_param->mtu);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,baud=%s,mtu=%u", name, path,
                invalid_baud_str, mtu);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;
}

class SerialTest : public ChannelTest,
    public WithParamInterface<std::tuple<speed_t, int>>
{
public:
    SerialTest() : ChannelTest::ChannelTest(),
        readerDevice("/dev/ttyUSB0"), writerDevice("/dev/ttyUSB1") {
    }

    void ChannelInit()
    {
        char opt[128];
        pirate_serial_param_t *param = &Reader.param.channel.serial;

        auto test_param = GetParam();
        const char *baud_str = NULL;
        speed_t baud = std::get<0>(test_param);
        unsigned mtu = std::get<0>(test_param);

        pirate_init_channel_param(SERIAL, &Reader.param);
        strncpy(param->path, readerDevice.c_str(), PIRATE_LEN_NAME - 1);
        if (baud) {
            param->baud = baud;
        } else {
            baud = SERIAL_DEFAULT_BAUD;
        }

        if (mtu) {
            param->mtu = mtu;
        } else {
            mtu = SERIAL_DEFAULT_MTU;
        }

        Writer.param = Reader.param;
        strncpy(Writer.param.channel.serial.path, writerDevice.c_str(), PIRATE_LEN_NAME - 1);

        switch (baud) {
        case B4800:   baud_str = "4800";   break;
        case B9600:   baud_str = "9600";   break;
        case B19200:  baud_str = "19200";  break;
        case B38400:  baud_str = "38400";  break;
        case B57600:  baud_str = "57600";  break;
        case B115200: baud_str = "115200"; break;
        case B230400: baud_str = "230400"; break;
        case B460800: baud_str = "460800"; break;
        default:
            FAIL() << "Invalid baud rate";
        }

        snprintf(opt, sizeof(opt) - 1, "serial,%s,baud=%s,mtu=%u", readerDevice.c_str(),
                    baud_str, mtu);
        Reader.desc.assign(opt);
        snprintf(opt, sizeof(opt) - 1, "serial,%s,baud=%s,mtu=%u", writerDevice.c_str(),
                    baud_str, mtu);
        Writer.desc.assign(opt);
    }

    static const speed_t TEST_BAUD = B115200;
    static const unsigned TEST_MTU = SERIAL_DEFAULT_MTU / 2;
    const std::string readerDevice;
    const std::string writerDevice;
};


TEST_P(SerialTest, Run)
{
    if ((access(readerDevice.c_str(), W_OK) == 0) &&
        (access(writerDevice.c_str(), R_OK) == 0)) {
        Run();
    } else {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
    }
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(SerialFunctionalTest, SerialTest,
    Combine(Values(0, SerialTest::TEST_BAUD),
            Values(0, SerialTest::TEST_MTU)));

} // namespace
