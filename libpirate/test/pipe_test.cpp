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
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;


TEST(ChannelPipeTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;
    const pirate_pipe_param_t *pipe_param = &param.channel.pipe;

    char opt[128];
    const char *name = "pipe";
    const char *path = "/tmp/test_pipe";

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%s", name, path);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(PIPE, param.channel_type);
    ASSERT_STREQ(path, pipe_param->path);
}

class PipeTest : public ChannelTest
{
public:
    void ChannelInit()
    {
        char opt[128];
        pirate_pipe_param_t *param = &Reader.param.channel.pipe;

        pirate_init_channel_param(PIPE, &Reader.param);
        strncpy(param->path, "/tmp/gaps.channel.test", PIRATE_LEN_NAME);
        Writer.param = Reader.param;

        snprintf(opt, sizeof(opt) - 1, "pipe,%s", param->path);
        Reader.desc.assign(opt);
        Writer.desc.assign(opt);
    }
};

TEST_F(PipeTest, Run)
{
    Run();
}

} // namespace
