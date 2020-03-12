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

TEST(ChannelUioTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_uio_param_t *uio_param = &param.channel.uio;

    char opt[128];
    const char *name = "uio";
    const char *path = "/tmp/test_uio";

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UIO_DEVICE, param.channel_type);
    ASSERT_STREQ("", uio_param->path);

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(UIO_DEVICE, param.channel_type);
    ASSERT_STREQ(path, uio_param->path);
}

class UioTest : public ChannelTest
{
public:
    void ChannelInit()
    {
        int rv;
        pirate_init_channel_param(UIO_DEVICE, &param);

        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // write and read parameters are the same
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }
};


TEST_F(UioTest, UioFunctionalTest)
{
    if (access("/dev/uio0", R_OK | W_OK) == 0) {
        Run();
    } else {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
    }
}

} // namespace
