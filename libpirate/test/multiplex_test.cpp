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
#include "libpirate_internal.h"
#include "channel_test.hpp"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::Values;
using ::testing::Combine;


TEST(ChannelMultiplexTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t param;

    char opt[128];
    const char *name = "multiplex";

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
}

class MultiplexTest : public ChannelTest
{
public:
    void ChannelInit() override
    {
        pirate_init_channel_param(MULTIPLEX, &Reader.param);
        Writer.param = Reader.param;
    }

    void TearDown() override
    {
        ChannelTest::TearDown();
        ASSERT_EQ(1, nonblocking_IO_attempt);
    }

    void ReaderChannelOpen() override
    {
        int flags, rv;
        flags = O_RDONLY;
        if (nonblocking_IO) {
            flags |= O_NONBLOCK;
        }

        Reader.gd = pirate_open_param(&Reader.param, flags);
        ASSERT_EQ(errno, 0);
        ASSERT_GE(Reader.gd, 0);

        rv = pirate_multiplex_open_parse(Reader.gd, "udp_socket,127.0.0.1,8080", flags, 2);
        ASSERT_EQ(errno, 0);
        ASSERT_GE(rv, 0);

        ASSERT_EQ(1, pirate_multiplex_count(Reader.gd));

        if (!nonblocking_IO) {
            rv = pirate_multiplex_open_parse(Reader.gd, "tcp_socket,127.0.0.1,8081", flags, 2);
            ASSERT_EQ(errno, 0);
            ASSERT_GE(rv, 0);
            ASSERT_EQ(3, pirate_multiplex_count(Reader.gd));
        }

        ReaderChannelPostOpen();

        rv = pthread_barrier_wait(&barrier);
        ASSERT_TRUE(rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD);
    }

    void WriterChannelOpen() override
    {
        int flags, rv;
        flags = O_WRONLY;
        if (nonblocking_IO) {
            flags |= O_NONBLOCK;
        }
        Writer.gd = pirate_open_param(&Writer.param, flags);
        ASSERT_EQ(errno, 0);
        ASSERT_GE(Writer.gd, 0);

        // simulate multiple writers with multiple calls to pirate_multiplex_open_parse()
        rv = pirate_multiplex_open_parse(Writer.gd, "udp_socket,127.0.0.1,8080", flags, 1);
        ASSERT_EQ(errno, 0);
        ASSERT_GE(rv, 0);

        rv = pirate_multiplex_open_parse(Writer.gd, "udp_socket,127.0.0.1,8080", flags, 1);
        ASSERT_EQ(errno, 0);
        ASSERT_GE(rv, 0);

        ASSERT_EQ(2, pirate_multiplex_count(Writer.gd));

        if (!nonblocking_IO) {
            rv = pirate_multiplex_open_parse(Writer.gd, "tcp_socket,127.0.0.1,8081", flags, 1);
            ASSERT_EQ(errno, 0);
            ASSERT_GE(rv, 0);

            rv = pirate_multiplex_open_parse(Writer.gd, "tcp_socket,127.0.0.1,8081", flags, 1);
            ASSERT_EQ(errno, 0);
            ASSERT_GE(rv, 0);

            ASSERT_EQ(4, pirate_multiplex_count(Writer.gd));
        }

        WriterChannelPostOpen();

        rv = pthread_barrier_wait(&barrier);
        ASSERT_TRUE(rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD);
    }

    void ReaderTest() override
    {
        int writers = (nonblocking_IO) ? 2 : 4;
        ReaderChannelOpen();

        memset(&stats_rd, 0, sizeof(stats_rd));

        for (size_t i = 0; i < len_size; i++)
        {
            if (nonblocking_IO)
            {
                int rv = sem_wait(&nonblocking_sem);
                EXPECT_EQ(0, errno);
                EXPECT_EQ(0, rv);
            }

            for (int j = 0; j < writers; j++)
            {
                ssize_t rv;
                ssize_t rl = len_arr[i].reader;
                ssize_t exp = MIN(len_arr[i].reader, len_arr[i].writer);

                memset(Reader.buf, 0xFA, rl);

                uint8_t *buf = Reader.buf;
                rv = pirate_read(Reader.gd, buf, rl);
                EXPECT_EQ(0, errno);
                EXPECT_EQ(rv, exp);
                EXPECT_TRUE(0 == std::memcmp(Writer.buf, Reader.buf, exp));

                stats_rd.packets++;
                stats_rd.bytes += exp;
            }

            BarrierWait();
        }

        if (nonblocking_IO)
        {
            ssize_t rv = pirate_read(Reader.gd, Reader.buf, 1);
            EXPECT_TRUE((errno == EAGAIN) || (errno == EWOULDBLOCK));
            EXPECT_EQ(rv, -1);
            errno = 0;
        }

        // barrier for nonblocking read test
        BarrierWait();

        ReaderChannelClose();
    }

};

TEST_F(MultiplexTest, Run)
{
    Run();
}

} // namespace
