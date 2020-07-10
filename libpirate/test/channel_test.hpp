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
#include <stdint.h>
#include <semaphore.h>
#include <sys/types.h>
#include <gtest/gtest.h>

namespace GAPS
{

class ChannelTest : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    virtual void WriteDataInit(ssize_t offset, ssize_t len);

    virtual void ChannelInit() = 0;

    virtual void WriterChannelOpen();
    virtual void ReaderChannelOpen();

    virtual void WriterChannelPostOpen() {}
    virtual void ReaderChannelPostOpen() {}

    virtual void WriterChannelPreClose() {}
    virtual void ReaderChannelPreClose() {}

    virtual void WriterChannelClose();
    virtual void ReaderChannelClose();

    virtual void WriterTest();
    virtual void ReaderTest();

    void Run();
    virtual void RunTestCase();

    struct TestPoint {
        TestPoint() : 
            gd(-1), buf(0), status(-1) {
            std::memset(&param, 0, sizeof(param));
        }

        int gd;
        uint8_t *buf;
        int status;
        pirate_channel_param_t param;
    } Writer, Reader;

    // Test lengths
    struct len_pair {
        ssize_t reader;
        ssize_t writer;
    };

    static const size_t len_size = 9;
    static const size_t buf_size = 32;

    struct len_pair len_arr[len_size] = {
        {1, 1}, {1, 2}, {2, 1},
        {8, 8}, {8, 16}, {16, 8},
        {1, 32}, {32, 1}, {32, 32}};

    // Reader writer synchronization
    pthread_barrier_t barrier;

    sem_t nonblocking_sem;

    // If true the producer and consumer
    // open the channel.
    // If false the producer and consumer
    // assume the channel is open.
    bool child_open;

    // If true then producer and consumer
    // use non-blocking I/O
    bool nonblocking_IO;

    // Channel statistics
    struct {
        uint32_t bytes;
        uint32_t packets;
    } stats_wr, stats_rd;
public:
    ChannelTest();
    static void *WriterThreadS(void *param);
    static void *ReaderThreadS(void *param);
};

class HalfClosedTest : public ChannelTest
{
protected:
    virtual void WriterTest() override;
    virtual void ReaderTest() override;
};

class ClosedWriterTest : public HalfClosedTest
{
public:
    virtual void RunTestCase() override;
};

class ClosedReaderTest : public HalfClosedTest
{
public:
    virtual void RunTestCase() override;
};

static const unsigned TEST_MIN_TX_LEN = 16;

} // namespace GAPS
