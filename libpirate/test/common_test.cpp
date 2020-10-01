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
#include <errno.h>
#include <gtest/gtest.h>
#include "libpirate.h"
#include "libpirate_internal.h"
#include "channel_test.hpp"

// Channel-type agnostic tests
namespace GAPS {

TEST(CommonChannel, InvalidOpen)
{
    pirate_channel_param_t param;
    int rv;

    pirate_init_channel_param(PIPE, &param);

    // Invalid flags
    rv = pirate_open_param(&param, O_RDWR);
    ASSERT_EQ(EINVAL, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;
}

TEST(CommonChannel, InvalidCLose)
{
    int rv;

    // Invalid channel number - negative
    rv = pirate_close(-1);
    ASSERT_EQ(EBADF, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    // Invalid channel number - exceeds bound
    rv = pirate_close(PIRATE_NUM_CHANNELS);
    ASSERT_EQ(EBADF, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;

    // Close unopened channel
    rv = pirate_close(0);
    ASSERT_EQ(EBADF, errno);
    ASSERT_EQ(-1, rv);
    errno = 0;
}

TEST(CommonChannel, InvalidReadWrite)
{
    int rv;
    uint8_t buf[16] = { 0 };

    // Read unopened channel
    rv = pirate_read(0, buf, sizeof(buf));
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EBADF, errno);
    errno = 0;

    // Write unopened channel
    rv = pirate_write(0, buf, sizeof(buf));
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EBADF, errno);
    errno = 0;
}

TEST(CommonChannel, UnparseChannelParam)
{
    char output[80];
    int rv;
    pirate_channel_param_t param;

    rv = pirate_parse_channel_param("device,/dev/null", &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_unparse_channel_param(&param, output, 17);
    ASSERT_EQ(16, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ("device,/dev/null", output);

    rv = pirate_unparse_channel_param(&param, output, 16);
    ASSERT_EQ(16, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ("device,/dev/nul", output);

    rv = pirate_unparse_channel_param(&param, output, 15);
    ASSERT_EQ(16, rv);
    ASSERT_EQ(0, errno);
    ASSERT_STREQ("device,/dev/nu", output);
}

TEST(CommonChannel, Stats)
{
    int rv, gd[2];
    char temp[80];
    const pirate_stats_t *stats_r, *stats_w;
    ssize_t nbytes;
    errno = 0;

    pirate_reset_stats();

    rv = pirate_pipe_parse(gd, "pipe,/tmp/pipe_gaps_stats", O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    nbytes = pirate_write(gd[1], "hello world", 12);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(nbytes, 12);

    nbytes = pirate_read(gd[0], temp, sizeof(temp));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(nbytes, 12);

    stats_r = pirate_get_stats(gd[0]);
    stats_w = pirate_get_stats(gd[1]);
    ASSERT_TRUE(stats_r != NULL);
    ASSERT_TRUE(stats_w != NULL);

    ASSERT_EQ(1u, stats_r->requests);
    ASSERT_EQ(12u, stats_r->bytes);
    ASSERT_EQ(1u, stats_r->success);
    ASSERT_EQ(0u, stats_r->fuzzed);
    ASSERT_EQ(0u, stats_r->errs);

    ASSERT_EQ(1u, stats_w->requests);
    ASSERT_EQ(12u, stats_w->bytes);
    ASSERT_EQ(1u, stats_r->success);
    ASSERT_EQ(0u, stats_w->fuzzed);
    ASSERT_EQ(0u, stats_w->errs);

    rv = pirate_close(gd[0]);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_close(gd[1]);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

}

TEST(CommonChannel, Drop)
{
    int rv, gd[2];
    char temp[80];
    const pirate_stats_t *stats_r, *stats_w;
    ssize_t nbytes;
    errno = 0;

    pirate_reset_stats();

    rv = pirate_pipe_parse(gd, "pipe,/tmp/pipe_gaps_stats,drop=2", O_RDWR);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    nbytes = pirate_write(gd[1], "hello", 6);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(nbytes, 6);

    nbytes = pirate_write(gd[1], "world", 6);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(nbytes, 6);

    nbytes = pirate_read(gd[0], temp, sizeof(temp));
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(nbytes, 6);
    ASSERT_STREQ("world", temp);

    stats_r = pirate_get_stats(gd[0]);
    stats_w = pirate_get_stats(gd[1]);
    ASSERT_TRUE(stats_r != NULL);
    ASSERT_TRUE(stats_w != NULL);

    ASSERT_EQ(1u, stats_r->requests);
    ASSERT_EQ(6u, stats_r->bytes);
    ASSERT_EQ(1u, stats_r->success);
    ASSERT_EQ(0u, stats_r->fuzzed);
    ASSERT_EQ(0u, stats_r->errs);

    ASSERT_EQ(2u, stats_w->requests);
    ASSERT_EQ(6u, stats_w->bytes);
    ASSERT_EQ(1u, stats_w->success);
    ASSERT_EQ(1u, stats_w->fuzzed);
    ASSERT_EQ(0u, stats_w->errs);

    rv = pirate_close(gd[0]);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

    rv = pirate_close(gd[1]);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);

}

} // namespace
