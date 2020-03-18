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
#include <stdlib.h>
#include "libpirate.h"
#include "channel_test.hpp"

namespace GAPS
{

ChannelTest::ChannelTest() : testing::Test() ,
    Writer({TEST_CHANNEL, NULL, -1}),  Reader({TEST_CHANNEL, NULL, -1}), 
    len({DEFAULT_START_LEN, DEFAULT_STOP_LEN, DEFAULT_STEP_LEN}),
    WriteDelayUs(0)
{

}

void ChannelTest::SetUp()
{
    int rv;
    errno = 0;

    Writer.buf = (uint8_t *) malloc(len.stop);
    ASSERT_NE(nullptr, Writer.buf);

    Reader.buf = (uint8_t *) malloc(len.stop);
    ASSERT_NE(nullptr, Reader.buf);

    rv = sem_init(&sem, 0, 0);
    ASSERT_EQ(0, rv);
}

void ChannelTest::TearDown()
{
    if (Writer.buf  != NULL)
    {
        free(Writer.buf );
        Writer.buf  = NULL;
    }

    if (Reader.buf != NULL)
    {
        free(Reader.buf);
        Reader.buf = NULL;
    }

    sem_destroy(&sem);
    errno = 0;
}

void ChannelTest::WriteDataInit(ssize_t len)
{
    for (ssize_t i = 0; i < len; ++i)
    {
        Writer.buf[i] = (i + len) & 0xFF; 
    }
}

void ChannelTest::WriterChannelOpen()
{
    int rv = pirate_open(Writer.channel, O_WRONLY);
    ASSERT_EQ(Writer.channel, rv);
    ASSERT_EQ(0, errno);
}

void ChannelTest::ReaderChannelOpen()
{
    int rv = pirate_open(Reader.channel, O_RDONLY);
    ASSERT_EQ(Reader.channel, rv);
    ASSERT_EQ(0, errno);
}

void ChannelTest::WriterChannelClose()
{
    int rv = pirate_close(Writer.channel, O_WRONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
}

void ChannelTest::ReaderChannelClose()
{
    int rv = pirate_close(Reader.channel, O_RDONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
}

void ChannelTest::Run()
{
    RunChildOpen(true);
    RunChildOpen(false);
}

void ChannelTest::RunChildOpen(bool child)
{
    int rv;
    pthread_t WriterId, ReaderId;
    void *WriterStatus, *ReaderStatus;

    childOpen = child;

    ChannelInit();

    if (!childOpen)
    {
        ASSERT_EQ(Reader.channel, Writer.channel);
        rv = pirate_pipe(Writer.channel, O_RDWR);
        ASSERT_EQ(Writer.channel, rv);
        ASSERT_EQ(0, errno);
    }

    rv = pthread_create(&ReaderId, NULL, ChannelTest::ReaderThreadS, this);
    ASSERT_EQ(0, rv);

    rv = pthread_create(&WriterId, NULL, ChannelTest::WriterThreadS, this);
    ASSERT_EQ(0, rv);

    rv = pthread_join(ReaderId, &ReaderStatus);
    ASSERT_EQ(0, rv);

    rv = pthread_join(WriterId, &WriterStatus);
    ASSERT_EQ(0, rv);
}

void *ChannelTest::WriterThreadS(void *param)
{
    ChannelTest *inst = static_cast<ChannelTest*>(param);
    inst->WriterTest();
    return NULL;
}

void *ChannelTest::ReaderThreadS(void *param)
{
    ChannelTest *inst = static_cast<ChannelTest*>(param);
    inst->ReaderTest();
    return NULL;
}

void ChannelTest::WriterTest()
{
    if (childOpen)
    {
        WriterChannelOpen();
    }

    for (ssize_t l = len.start; l < len.stop; l += len.step)
    {
        int sts;
        ssize_t rv;

        WriteDataInit(l);

        if (WriteDelayUs)
        {
            struct timespec ts;
            ts.tv_sec = WriteDelayUs / 1000000;
            ts.tv_nsec = (WriteDelayUs % 1000000) * 1000;
            nanosleep(&ts, NULL);
        }

        rv = pirate_write(Writer.channel, Writer.buf, l);
        ASSERT_EQ(l, rv);
        ASSERT_EQ(0, errno);

        sts = sem_wait(&sem);
        ASSERT_EQ(0, sts);
    }

    WriterChannelClose();
}

void ChannelTest::ReaderTest()
{
    if (childOpen)
    {
        ReaderChannelOpen();
    }

    for (ssize_t l = len.start; l < len.stop; l += len.step)
    {
        int sts;
        ssize_t rv;

        memset(Reader.buf, 0xFA, l);

        ssize_t remain = l;
        uint8_t *buf = Reader.buf;
        do {
            rv = pirate_read(Reader.channel, buf, remain);
            ASSERT_GT(rv, 0);
            ASSERT_EQ(0, errno);
            remain -= rv;
            buf += rv;

        } while (remain > 0);
        EXPECT_TRUE(0 == std::memcmp(Writer.buf, Reader.buf, l));

        sts = sem_post(&sem);
        ASSERT_EQ(0, sts);
        pthread_yield();
    }

    ReaderChannelClose();
}

} // namespace
