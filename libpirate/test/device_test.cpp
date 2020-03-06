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

#include <errno.h>
#include <string>
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS {

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;

TEST(ChannelDeviceTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_device_param_t *device_param = &param.device;

    char opt[128];
    const char *name = "device";
    const char *path = "/tmp/test_device";
    const unsigned iov_len = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(DEVICE, param.channel_type);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, device_param->path);
    ASSERT_EQ(0u, device_param->iov_len);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,%u", name, path, iov_len);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(DEVICE, param.channel_type);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, device_param->path);
    ASSERT_EQ(iov_len, device_param->iov_len);
}

class DeviceTest : public ChannelTest, public WithParamInterface<int>
{
public:
    void ChannelInit() {
        int rv;
        pirate_init_channel_param(DEVICE, &param);
        snprintf(param.device.path, PIRATE_LEN_NAME - 1, "/tmp/gaps_dev");
        param.device.iov_len = GetParam();

        if (mkfifo(param.device.path, 0660) == -1) {
            ASSERT_EQ(EEXIST, errno);
            errno = 0;
        }

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }
};

TEST_P(DeviceTest, Run)
{
    Run();
}

// Test with IO vector sizes 0 and 16, passed as parameters
INSTANTIATE_TEST_SUITE_P(DeviceFunctionalTest, DeviceTest,
    Values(0, ChannelTest::TEST_IOV_LEN));

} // namespace
