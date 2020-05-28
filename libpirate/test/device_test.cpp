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

#include <string>
#include <errno.h>
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS {

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;

TEST(ChannelDeviceTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_device_param_t *device_param = &param.channel.device;

    char opt[128];
    const char *name = "device";
    const char *path = "/tmp/test_device";
    const unsigned min_tx = 42;

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(DEVICE, param.channel_type);
    ASSERT_STREQ(path, device_param->path);
    ASSERT_EQ(0u, device_param->min_tx);

    snprintf(opt, sizeof(opt) - 1, "%s,%s,min_tx_size=%u", name, path, min_tx);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(DEVICE, param.channel_type);
    ASSERT_STREQ(path, device_param->path);
    ASSERT_EQ(min_tx, device_param->min_tx);
}

class DeviceTest : public ChannelTest, public WithParamInterface<int> {
public:
    void ChannelInit()
    {
        pirate_device_param_t *param = &Reader.param.channel.device;
        
        pirate_init_channel_param(DEVICE, &Reader.param);
        snprintf(param->path, PIRATE_LEN_NAME - 1, "/tmp/gaps_dev");
        param->min_tx = GetParam();
        Writer.param = Reader.param;

        if (mkfifo(param->path, 0660) == -1) {
            ASSERT_EQ(EEXIST, errno);
            errno = 0;
        }
    }
};

TEST_P(DeviceTest, Run)
{
    Run();
}

INSTANTIATE_TEST_SUITE_P(DeviceFunctionalTest, DeviceTest,
    Values(0, TEST_MIN_TX_LEN));

} // namespace
