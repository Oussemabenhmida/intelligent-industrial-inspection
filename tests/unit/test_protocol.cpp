#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "firmware/hal/crc.h"
#include "firmware/hal/hal_types.h"
#include "protocol/packet.h"
}

static void setup(Packet *tx, Packet *rx, u8 *buf, u16 buf_size)
{
    CRC16_Init();
    memset(tx,  0, sizeof(Packet));
    memset(rx,  0, sizeof(Packet));
    memset(buf, 0, buf_size);
}

TEST(ProtocolTest, InspectionSerializeDeserialize)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0U, 97U, 12345U);
    u16 written = PROTO_Serialize(&tx, buf, sizeof(buf));
    EXPECT_GT(written, 0U);

    ParseResult r = PROTO_Deserialize(buf, written, &rx);
    EXPECT_EQ(r, PARSE_OK);
    EXPECT_EQ(rx.header.sof,     (u8)PROTO_SOF);
    EXPECT_EQ(rx.header.version, (u8)PROTO_VERSION);
    EXPECT_EQ(rx.header.seq,     1U);
    EXPECT_EQ(rx.header.msg_id,  (u8)MSG_INSPECTION);

    InspectionPayload *p = (InspectionPayload *)rx.payload;
    EXPECT_EQ(p->result,       (u8)INSPECT_GOOD);
    EXPECT_EQ(p->confidence,   97U);
    EXPECT_EQ(p->timestamp_ms, 12345U);
}

TEST(ProtocolTest, CorruptionDetected)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0U, 97U, 12345U);
    u16 written = PROTO_Serialize(&tx, buf, sizeof(buf));
    buf[7] ^= 0xFF;
    ParseResult r = PROTO_Deserialize(buf, written, &rx);
    EXPECT_EQ(r, PARSE_BAD_CRC);
}

TEST(ProtocolTest, BadSofDetected)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0U, 97U, 12345U);
    u16 written = PROTO_Serialize(&tx, buf, sizeof(buf));
    buf[0] = 0x00U;
    ParseResult r = PROTO_Deserialize(buf, written, &rx);
    EXPECT_EQ(r, PARSE_BAD_SOF);
}

TEST(ProtocolTest, BadVersionDetected)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0U, 97U, 12345U);
    u16 written = PROTO_Serialize(&tx, buf, sizeof(buf));
    buf[1] = 0x99U;
    ParseResult r = PROTO_Deserialize(buf, written, &rx);
    EXPECT_EQ(r, PARSE_BAD_VERSION);
}

TEST(ProtocolTest, HeartbeatRoundTrip)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildHeartbeat(&tx, 5U);
    u16 written = PROTO_Serialize(&tx, buf, sizeof(buf));
    EXPECT_EQ(written, (u16)(PACKET_HEADER_SIZE + PACKET_CRC_SIZE));

    ParseResult r = PROTO_Deserialize(buf, written, &rx);
    EXPECT_EQ(r, PARSE_OK);
    EXPECT_EQ(rx.header.msg_id, (u8)MSG_HEARTBEAT);
    EXPECT_EQ(rx.header.length, 0U);
}

TEST(ProtocolTest, SequenceNumberValid)
{
    EXPECT_TRUE(PROTO_IsValidSeq(5U, 5U));
    EXPECT_FALSE(PROTO_IsValidSeq(5U, 6U));
    EXPECT_FALSE(PROTO_IsValidSeq(5U, 4U));
}

TEST(ProtocolTest, NullGuards)
{
    Packet rx;
    u8 buf[PACKET_MAX_SIZE];
    CRC16_Init();
    memset(&rx, 0, sizeof(rx));

    u16 written = PROTO_Serialize(NULL, buf, sizeof(buf));
    EXPECT_EQ(written, 0U);

    ParseResult r = PROTO_Deserialize(NULL, 10U, &rx);
    EXPECT_EQ(r, PARSE_NULL);
}

TEST(ProtocolTest, TooShortBufferDetected)
{
    Packet tx, rx;
    u8 buf[PACKET_MAX_SIZE];
    setup(&tx, &rx, buf, sizeof(buf));

    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0U, 97U, 12345U);
    PROTO_Serialize(&tx, buf, sizeof(buf));
    ParseResult r = PROTO_Deserialize(buf, 2U, &rx);
    EXPECT_EQ(r, PARSE_BAD_LENGTH);
}
