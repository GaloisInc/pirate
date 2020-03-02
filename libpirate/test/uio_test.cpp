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
TEST(ChannelUioTest, Configuration)
{
    const int channel = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;

    // Default configuration
    pirate_channel_param_t param;
    pirate_uio_param_t *uio_param = &param.uio;
    int rv = pirate_init_channel_param(UIO_DEVICE, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_UIO_DEVICE, uio_param->path);

    // Apply configuration
    const char *test_path = "/tmp/test_uio_path";
    strncpy(uio_param->path, test_path, sizeof(uio_param->path) - 1);

    rv = pirate_set_channel_param(UIO_DEVICE, channel, flags, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    pirate_channel_param_t param_get;
    pirate_uio_param_t *uio_param_get = &param_get.uio;
    memset(uio_param_get, 0, sizeof(*uio_param_get));

    channel_t ch =  pirate_get_channel_param(channel, flags, &param_get);
    ASSERT_EQ(UIO_DEVICE, ch);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(test_path, uio_param_get->path);
}

TEST(ChannelUioTest, ConfigurationParser) {
    const int ch_num = ChannelTest::TEST_CHANNEL;
    const int flags = O_RDONLY;
    pirate_channel_param_t param;
    const pirate_uio_param_t *uio_param = &param.uio;
    channel_t channel;

    char opt[128];
    const char *name = "uio";
    const char *path = "/tmp/test_uio";

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s", name);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UIO_DEVICE, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(DEFAULT_UIO_DEVICE, uio_param->path);

    memset(&param, 0, sizeof(param));
    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    channel = pirate_parse_channel_param(ch_num, flags, opt, &param);
    ASSERT_EQ(UIO_DEVICE, channel);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ(path, uio_param->path);
}

class UioTest : public ChannelTest
{
public:
    void ChannelInit()
    {
        int rv = pirate_init_channel_param(UIO_DEVICE, Writer.channel, O_WRONLY,
                                            &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        rv = pirate_set_channel_param(UIO_DEVICE, Writer.channel, O_WRONLY,
                                        &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(UIO_DEVICE, Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }
};


TEST_F(UioTest, UioFunctionalTest)
{
    if (access("/dev/uio0", R_OK | W_OK) == 0) {
        Run();
    }
}

} // namespace