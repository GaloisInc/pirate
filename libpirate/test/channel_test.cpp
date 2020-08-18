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

#include <stdlib.h>
#ifdef _WIN32
#include "windows_port.hpp"
#include "windows/libpirate.h"
#include "windows/libpirate_internal.h"
#else
#include "libpirate.h"
#include "libpirate_internal.h"
#endif
#include "channel_test.hpp"
#include "cross_platform_test.hpp"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

namespace GAPS
{

ChannelTest::ChannelTest() : testing::Test(), nonblocking_IO_attempt(false) { }

void ChannelTest::SetUp()
{
    CROSS_PLATFORM_RESET_ERROR();

    Writer.buf = (uint8_t *) malloc(buf_size);
    ASSERT_NE(nullptr, Writer.buf);

    Reader.buf = (uint8_t *) malloc(buf_size);
    ASSERT_NE(nullptr, Reader.buf);

#ifdef _WIN32
    BOOL success = InitializeSynchronizationBarrier(&barrier, 2, 0);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(TRUE, success);

    nonblocking_sem = CreateSemaphore(NULL, 0, 1, NULL);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_TRUE(nonblocking_sem != nullptr);
#else
    int rv = pthread_barrier_init(&barrier, NULL, 2);
    ASSERT_EQ(0, rv);

    rv = sem_init(&nonblocking_sem, 0, 0);
    ASSERT_EQ(0, rv);
#endif

    pirate_reset_gd();
}

void ChannelTest::TearDown()
{
    if (Writer.buf != NULL)
    {
        free(Writer.buf);
        Writer.buf  = NULL;
    }

    if (Reader.buf != NULL)
    {
        free(Reader.buf);
        Reader.buf = NULL;
    }


#ifdef _WIN32
    DeleteSynchronizationBarrier(&barrier);
    CloseHandle(nonblocking_sem);
#else
    pthread_barrier_destroy(&barrier);
    sem_destroy(&nonblocking_sem);
#endif
    CROSS_PLATFORM_RESET_ERROR();
}

void ChannelTest::WriteDataInit(ssize_t offset, ssize_t len)
{
    for (ssize_t i = 0; i < len; ++i)
    {
        Writer.buf[i] = (offset + i) & 0xFF;
    }
}

void ChannelTest::WriterChannelOpen()
{
    int flags, rv;
    char desc[256];
    pirate_channel_param_t temp_param;

    rv = pirate_unparse_channel_param(&Writer.param, desc, sizeof(desc) - 1);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_GT(rv, 0);

    rv = pirate_parse_channel_param(desc, &temp_param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, memcmp(&Writer.param, &temp_param, sizeof(pirate_channel_param_t)));

    flags = O_WRONLY;
    if (nonblocking_IO) {
        flags |= O_NONBLOCK;
    }
    Writer.gd = pirate_open_param(&Writer.param, flags);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_GE(Writer.gd, 0);

    WriterChannelPostOpen();

#ifdef _WIN32
    EnterSynchronizationBarrier(&barrier, 0);
#else
    rv = pthread_barrier_wait(&barrier);
    ASSERT_TRUE(rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD);
#endif
}

void ChannelTest::ReaderChannelOpen()
{
    int flags, rv;
    char desc[256];
    pirate_channel_param_t temp_param;

    rv = pirate_unparse_channel_param(&Reader.param, desc, sizeof(desc) - 1);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_GT(rv, 0);

    rv = pirate_parse_channel_param(desc, &temp_param);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, memcmp(&Reader.param, &temp_param, sizeof(pirate_channel_param_t)));

    flags = O_RDONLY;
    if (nonblocking_IO) {
        flags |= O_NONBLOCK;
    }
    Reader.gd = pirate_open_param(&Reader.param, flags);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_GE(Reader.gd, 0);

    ReaderChannelPostOpen();

#ifdef _WIN32
    EnterSynchronizationBarrier(&barrier, 0);
#else
    rv = pthread_barrier_wait(&barrier);
    ASSERT_TRUE(rv == 0 || rv == PTHREAD_BARRIER_SERIAL_THREAD);
#endif
}

void ChannelTest::WriterChannelClose()
{
    int rv;
    
    WriterChannelPreClose();

    rv = pirate_close(Writer.gd);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
}

void ChannelTest::ReaderChannelClose()
{
    int rv;

    ReaderChannelPreClose();
    
    rv = pirate_close(Reader.gd);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(0, rv);
}

void ChannelTest::Run()
{
    // init the channel type
    ChannelInit();
    ASSERT_NE(INVALID, Reader.param.channel_type);
    ASSERT_EQ(Reader.param.channel_type, Writer.param.channel_type);
    ssize_t mtu = pirate_write_mtu_estimate(&Writer.param);
    ASSERT_GE(mtu, 0);
    for(int child = 0; child <= 1; child++)
    {
        for (int nonblock = 0; nonblock <= 1; nonblock++)
        {
            child_open = (child == 1);
            nonblocking_IO = (nonblock == 1);
            if (nonblocking_IO)
            {
                if (pirate_nonblock_channel_type(Writer.param.channel_type, mtu))
                {
                    nonblocking_IO_attempt = true;
                }
                else
                {
                    continue;
                }
            }
            if (!child_open && !pirate_pipe_channel_type(Writer.param.channel_type))
            {
                continue;
            }
            RunTestCase();
        }
    }
}

void ChannelTest::RunTestCase()
{
    // re-init properties that are affected by
    // child_open and nonblocking_IO
    ChannelInit();

    if (!child_open)
    {
        int rv, gd[2] = {-1, -1};
        int flags = O_RDWR;
        if (nonblocking_IO) {
            flags |= O_NONBLOCK;
        }
        rv = pirate_pipe_param(gd, &Writer.param, flags);
        // memcpy() to keep reader and writer params in sync
        // Not needed for correctness of execution
        // Might be necessary if we compare these values in a test assertion
        memcpy(&Reader.param.channel, &Writer.param.channel, sizeof(Writer.param.channel));
        ASSERT_CROSS_PLATFORM_NO_ERROR();
        ASSERT_EQ(0, rv);
        ASSERT_GE(gd[0], 0);
        ASSERT_GE(gd[1], 0);
        Reader.gd = gd[0];
        Writer.gd = gd[1];
    }

#ifdef _WIN32
    HANDLE  hThreadArray[2];
    hThreadArray[0] = CreateThread(NULL, 0, ChannelTest::ReaderThreadS, this, 0, NULL);
    hThreadArray[1] = CreateThread(NULL, 0, ChannelTest::WriterThreadS, this, 0, NULL);
    WaitForMultipleObjects(2, hThreadArray, TRUE, INFINITE);
    CloseHandle(hThreadArray[0]);
    CloseHandle(hThreadArray[1]);
#else
    int rv;
    pthread_t WriterId, ReaderId;
    void *WriterStatus, *ReaderStatus;

    rv = pthread_create(&ReaderId, NULL, ChannelTest::ReaderThreadS, this);
    ASSERT_EQ(0, rv);

    rv = pthread_create(&WriterId, NULL, ChannelTest::WriterThreadS, this);
    ASSERT_EQ(0, rv);

    rv = pthread_join(ReaderId, &ReaderStatus);
    ASSERT_EQ(0, rv);

    rv = pthread_join(WriterId, &WriterStatus);
    ASSERT_EQ(0, rv);
#endif
}

#ifdef _WIN32
DWORD WINAPI ChannelTest::WriterThreadS(LPVOID param)
#else
void *ChannelTest::WriterThreadS(void *param)
#endif
{
    CROSS_PLATFORM_RESET_ERROR();
    ChannelTest *inst = static_cast<ChannelTest*>(param);
    inst->WriterTest();
    return NULL;
}

#ifdef _WIN32
DWORD WINAPI ChannelTest::ReaderThreadS(LPVOID param)
#else
void* ChannelTest::ReaderThreadS(void* param)
#endif
{
    CROSS_PLATFORM_RESET_ERROR();
    ChannelTest *inst = static_cast<ChannelTest*>(param);
    inst->ReaderTest();
    return NULL;
}

void ChannelTest::BarrierWait()
{
#ifdef _WIN32
    EnterSynchronizationBarrier(&barrier, 0);
#else
    int sts = pthread_barrier_wait(&barrier);
    EXPECT_TRUE(sts == 0 || sts == PTHREAD_BARRIER_SERIAL_THREAD);
#endif
}

void ChannelTest::WriterTest()
{
    ssize_t offset = 0;
    if (child_open)
    {
        WriterChannelOpen();
    }

    memset(&stats_wr, 0, sizeof(stats_wr));

    for (size_t i = 0; i < len_size; i++)
    {
        ssize_t rv;
        ssize_t wl = len_arr[i].writer;

        WriteDataInit(offset, wl);
        offset += wl + 1;

        rv = pirate_write(Writer.gd, Writer.buf, wl);
        EXPECT_EQ(wl, rv);
        ASSERT_CROSS_PLATFORM_NO_ERROR();

        if (nonblocking_IO)
        {
#ifdef _WIN32
            BOOL rv_sem = ReleaseSemaphore(nonblocking_sem, 1, NULL);
            EXPECT_NE(0, rv_sem);
#else
            rv = sem_post(&nonblocking_sem);
            EXPECT_EQ(0, errno);
            EXPECT_EQ(0, rv);
#endif
        }

        stats_wr.packets++;
        stats_wr.bytes += wl;

        BarrierWait();
    }

    // barrier for nonblocking read test
    BarrierWait();

    WriterChannelClose();
}

void ChannelTest::ReaderTest()
{
    if (child_open)
    {
        ReaderChannelOpen();
    }

    memset(&stats_rd, 0, sizeof(stats_rd));

    for (size_t i = 0; i < len_size; i++)
    {
        ssize_t rv;
        ssize_t rl = len_arr[i].reader;
        ssize_t exp = MIN(len_arr[i].reader, len_arr[i].writer);

        memset(Reader.buf, 0xFA, rl);

        if (nonblocking_IO)
        {
#ifdef _WIN32
            DWORD rv_wait = WaitForSingleObject(nonblocking_sem, INFINITE);
            EXPECT_EQ(WAIT_OBJECT_0, rv_wait);
#else
            rv = sem_wait(&nonblocking_sem);
            EXPECT_EQ(0, errno);
            EXPECT_EQ(0, rv);
#endif
        }

        uint8_t *buf = Reader.buf;
        rv = pirate_read(Reader.gd, buf, rl);
        ASSERT_CROSS_PLATFORM_NO_ERROR();
        EXPECT_EQ(rv, exp);
        EXPECT_TRUE(0 == std::memcmp(Writer.buf, Reader.buf, exp));

        stats_rd.packets++;
        stats_rd.bytes += exp;

        BarrierWait();
    }

    if (nonblocking_IO)
    {
        ssize_t rv = pirate_read(Reader.gd, Reader.buf, 1);
#ifdef _WIN32
        EXPECT_EQ(GetLastError(), WSAEWOULDBLOCK);
#else
        EXPECT_TRUE((errno == EAGAIN) || (errno == EWOULDBLOCK));
#endif
        EXPECT_EQ(rv, -1);
        errno = 0;
    }

    // barrier for nonblocking read test
    BarrierWait();

    ReaderChannelClose();
}

void HalfClosedTest::ReaderTest()
{
    if (child_open)
    {
        ReaderChannelOpen();
    }
}

void HalfClosedTest::WriterTest()
{
    if (child_open)
    {
        WriterChannelOpen();
    }
}

void ClosedWriterTest::RunTestCase()
{
    ssize_t rv;

    ChannelTest::RunTestCase();

    WriterChannelClose();

    rv = pirate_read(Reader.gd, Reader.buf, buf_size);
    ASSERT_CROSS_PLATFORM_NO_ERROR();
    ASSERT_EQ(rv, 0);

    ReaderChannelClose();
}

void ClosedReaderTest::RunTestCase()
{
    ssize_t nbytes;

    ChannelTest::RunTestCase();

#ifndef _WIN32
    int rv;
    struct sigaction new_action, prev_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = SIG_IGN;
    new_action.sa_flags = 0;

    rv = sigaction(SIGPIPE, &new_action, &prev_action);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);
#endif

    ReaderChannelClose();

    WriteDataInit(0, buf_size);
    nbytes = pirate_write(Writer.gd, Writer.buf, buf_size);
    ASSERT_EQ(errno, EPIPE);
    ASSERT_EQ(nbytes, -1);
    errno = 0;

    WriterChannelClose();

#ifndef _WIN32
    rv = sigaction(SIGPIPE, &prev_action, NULL);
    ASSERT_EQ(errno, 0);
    ASSERT_EQ(rv, 0);
#endif
}


} // namespace
