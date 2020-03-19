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
#include "libpirate.h"
#include "mercury_cntl.h"
#include "channel_test.hpp"

namespace GAPS
{

using ::testing::WithParamInterface;
using ::testing::TestWithParam;
using ::testing::ValuesIn;

TEST(ChannelMercuryTest, ConfigurationParser) {
    int rv;
    pirate_channel_param_t expParam, rdParam;
    char opt[256];
    const char *name = "mercury";
    const uint32_t level = 1;
    const uint32_t src_id = 2;
    const uint32_t dst_id = 3;
    const uint32_t timeout_ms = 2000;
    const uint32_t msg_ids[] = { 4, 5, 6 };

    pirate_init_channel_param(MERCURY, &expParam);
    pirate_init_channel_param(MERCURY, &rdParam);

    snprintf(opt, sizeof(opt) - 1, "%s", name);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;
    
    snprintf(opt, sizeof(opt) - 1, "%s,%u", name, level);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;
 
    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u", name, level, src_id);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(-1, rv);
    ASSERT_EQ(EINVAL, errno);
    errno = 0;

    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u,%u", name, level, src_id, dst_id);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    expParam.channel.mercury.session.level = level;
    expParam.channel.mercury.session.source_id = src_id;
    expParam.channel.mercury.session.destination_id = dst_id;
    expParam.channel.mercury.mtu = PIRATE_MERCURY_DEFAULT_MTU;
    expParam.channel.mercury.timeout_ms = PIRATE_MERCURY_DEFAULT_TIMEOUT_MS;
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));

    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u,%u,%u", name, level, src_id,
            dst_id, timeout_ms);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    expParam.channel.mercury.timeout_ms = timeout_ms;
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));

    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u,%u,%u,%u", name, level, src_id,
            dst_id, timeout_ms, msg_ids[0]);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    expParam.channel.mercury.session.message_count = 1;
    expParam.channel.mercury.session.messages[0] = msg_ids[0];
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));

    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u,%u,%u,%u,%u", name, level, src_id,
            dst_id, timeout_ms, msg_ids[0], msg_ids[1]);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    expParam.channel.mercury.session.message_count = 2;
    expParam.channel.mercury.session.messages[1] = msg_ids[1];
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));

    snprintf(opt, sizeof(opt) - 1, "%s,%u,%u,%u,%u,%u,%u,%u", name, level,
            src_id, dst_id, timeout_ms, msg_ids[0], msg_ids[1], msg_ids[2]);
    rv = pirate_parse_channel_param(opt, &rdParam);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
    expParam.channel.mercury.session.message_count = 3;
    expParam.channel.mercury.session.messages[2] = msg_ids[2];
    EXPECT_TRUE(0 == std::memcmp(&expParam, &rdParam, sizeof(rdParam)));
}

TEST(ChannelMercuryTest, DefaultSession) {
    int rv = 0;
    const int channel = 0;

    const uint32_t session_id = 1;
    const uint8_t wr_data[] = { 0xC0, 0xDE, 0xDA, 0xDA };
    const ssize_t data_len = sizeof(wr_data);
    uint8_t rd_data[data_len] = { 0 };
    ssize_t io_size = -1;
    mercury_dev_stat_t stats;
    memset(&stats, 0, sizeof(stats));

    if (access(PIRATE_MERCURY_ROOT_DEV, R_OK | W_OK) != 0) {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
        return;
    }

    pirate_channel_param_t param;
    pirate_init_channel_param(MERCURY, &param);

    rv =  pirate_set_channel_param(channel, O_WRONLY, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv =  pirate_set_channel_param(channel, O_RDONLY, &param);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_open(channel, O_WRONLY);
    ASSERT_EQ(channel, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_open(channel, O_RDONLY);
    ASSERT_EQ(channel, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_get_channel_param(channel, O_WRONLY, &param);
    ASSERT_EQ(0, errno);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(session_id, param.channel.mercury.session.id);

    io_size = pirate_write(channel, wr_data, data_len);
    ASSERT_EQ(io_size, data_len);
    ASSERT_EQ(0, errno);

    io_size = pirate_read(channel, rd_data, data_len);
    ASSERT_EQ(io_size, data_len);
    ASSERT_EQ(0, errno);

    EXPECT_TRUE(0 == std::memcmp(wr_data, rd_data, data_len));

    rv = mercury_cmd_stat(session_id, &stats);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    ASSERT_EQ(1u, stats.send_count);
    ASSERT_EQ(1u, stats.receive_count);
    ASSERT_EQ(0u, stats.send_reject_count);
    ASSERT_EQ(0u, stats.receive_reject_count);
    ASSERT_EQ(1u, stats.send_ilip_count);
    ASSERT_EQ(1u, stats.receive_ilip_count);
    ASSERT_EQ(0u, stats.send_ilip_reject_count);
    ASSERT_EQ(0u, stats.receive_ilip_reject_count);

    rv = mercury_cmd_stat_clear(session_id);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = mercury_cmd_stat(session_id, &stats);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    ASSERT_EQ(0u, stats.send_count);
    ASSERT_EQ(0u, stats.receive_count);
    ASSERT_EQ(0u, stats.send_reject_count);
    ASSERT_EQ(0u, stats.receive_reject_count);
    ASSERT_EQ(0u, stats.send_ilip_count);
    ASSERT_EQ(0u, stats.receive_ilip_count);
    ASSERT_EQ(0u, stats.send_ilip_reject_count);
    ASSERT_EQ(0u, stats.receive_ilip_reject_count);

    rv = pirate_close(channel, O_WRONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);

    rv = pirate_close(channel, O_RDONLY);
    ASSERT_EQ(0, rv);
    ASSERT_EQ(0, errno);
}


typedef struct {
    int         channel;
    uint32_t    level;
    uint32_t    source_id;
    uint32_t    destination_id;
    uint32_t    message_count;
    uint32_t    messages[5];
    uint32_t    session_id;   // Expected session ID
} MercuryTestParam;

class MercuryTest : public ChannelTest, public WithParamInterface<MercuryTestParam>
{
public:
    void ChannelInit() override
    {
        WriteDelayUs = 10000;

        int rv;
        mMercuryParam = GetParam();
        Writer.channel = Reader.channel = mMercuryParam.channel;

        // Writer
        pirate_init_channel_param(MERCURY, &param);
        param.channel.mercury.session.level          = mMercuryParam.level;
        param.channel.mercury.session.source_id      = mMercuryParam.source_id;
        param.channel.mercury.session.destination_id = mMercuryParam.destination_id;
        param.channel.mercury.session.message_count  = mMercuryParam.message_count;
        for (uint32_t i = 0; i < param.channel.mercury.session.message_count; ++i) {
            param.channel.mercury.session.messages[i] = mMercuryParam.messages[i];
        }
        rv = pirate_set_channel_param(Writer.channel, O_WRONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        // Reader
        pirate_init_channel_param(MERCURY, &param);
        param.channel.mercury.session.level          = mMercuryParam.level;
        param.channel.mercury.session.source_id      = mMercuryParam.source_id;
        param.channel.mercury.session.destination_id = mMercuryParam.destination_id;
        param.channel.mercury.session.message_count  = mMercuryParam.message_count;
        for (uint32_t i = 0; i < param.channel.mercury.session.message_count; ++i) {
            param.channel.mercury.session.messages[i] = mMercuryParam.messages[i];
        }
        rv = pirate_set_channel_param(Reader.channel, O_RDONLY, &param);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    void WriterChannelOpen() override
    {
        pirate_channel_param_t rdParam;

        int rv = pirate_open(Writer.channel, O_WRONLY);
        ASSERT_EQ(Writer.channel, rv);
        ASSERT_EQ(0, errno);

        rv = pirate_get_channel_param(Writer.channel, O_WRONLY, &rdParam);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        ASSERT_EQ(MERCURY, rdParam.channel_type);
        ASSERT_EQ(mMercuryParam.session_id, rdParam.channel.mercury.session.id);

        rv = mercury_cmd_stat_clear(rdParam.channel.mercury.session.id);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    void ReaderChannelOpen() override
    {
        pirate_channel_param_t rdParam;

        int rv = pirate_open(Reader.channel, O_RDONLY);
        ASSERT_EQ(Reader.channel, rv);
        ASSERT_EQ(0, errno);

        rv = pirate_get_channel_param(Writer.channel, O_RDONLY, &rdParam);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        ASSERT_EQ(MERCURY, rdParam.channel_type);
        ASSERT_EQ(mMercuryParam.session_id, rdParam.channel.mercury.session.id);

        rv = mercury_cmd_stat_clear(rdParam.channel.mercury.session.id);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);
    }

    void WriterChannelClose() override {
        mercury_dev_stat_t test_stats;
        int rv = mercury_cmd_stat(mMercuryParam.session_id, &test_stats);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        ASSERT_EQ(statsWr.packets, test_stats.send_count);
        ASSERT_EQ(0u, test_stats.send_reject_count);
        ASSERT_EQ(statsWr.packets, test_stats.send_ilip_count);
        ASSERT_EQ(0u, test_stats.send_ilip_reject_count);

        ChannelTest::WriterChannelClose();
    }

    void ReaderChannelClose() override {
        mercury_dev_stat_t test_stats;
        int rv = mercury_cmd_stat(mMercuryParam.session_id, &test_stats);
        ASSERT_EQ(0, rv);
        ASSERT_EQ(0, errno);

        ASSERT_EQ(statsRd.packets, test_stats.receive_count);
        ASSERT_EQ(0u, test_stats.receive_reject_count);
        ASSERT_EQ(statsRd.packets, test_stats.receive_ilip_count);
        ASSERT_EQ(0u, test_stats.receive_ilip_reject_count);

        ChannelTest::ReaderChannelClose();
    }

protected:
    MercuryTestParam mMercuryParam;
};

TEST_P(MercuryTest, Run)
{
    if (access(PIRATE_MERCURY_ROOT_DEV, R_OK | W_OK) == 0) {
        Run();
    } else {
        ASSERT_EQ(ENOENT, errno);
        errno = 0;
    }
}

static MercuryTestParam MercuryParams [] = 
{
    // CH,LVL,SRC,DST,MSG_CNT,[5 MSGS],        EXP SESSION_ID
    {  0, 1,  1,  0,  0,      {0, 0, 0, 0, 0}, 0x00000001},
    {  2, 2,  2,  0,  0,      {0, 0, 0, 0, 0}, 0x00000002},
    {  1, 1,  1,  2,  5,      {1, 3, 0, 0, 0}, 0xECA51756},
    {  3, 2,  2,  1,  5,      {1, 3, 0, 0, 0}, 0x67FF90F4},
    {  0, 1,  1,  2,  5,      {1, 6, 5, 0, 0}, 0x6BB83E13},
    {  1, 2,  2,  1,  5,      {2, 3, 4, 0, 0}, 0x8127AA5B},
    {  2, 1,  1,  2,  5,      {1, 1, 3, 4, 0}, 0x2C2B8E86},
    {  3, 2,  2,  1,  5,      {2, 1, 1, 2, 0}, 0x442D2490},
    {  0, 1,  1,  2,  5,      {1, 2, 5, 0, 0}, 0xBC5A32FB},
    {  1, 2,  2,  1,  5,      {2, 1, 3, 4, 0}, 0x574C9A21},
};

INSTANTIATE_TEST_SUITE_P(MercuryFunctionalTest, MercuryTest,
    ValuesIn(MercuryParams));

} // namespace
