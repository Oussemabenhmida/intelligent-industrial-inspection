#include <gtest/gtest.h>

extern "C" {
#include "firmware/hal/crc.h"
}

class CrcTest : public ::testing::Test {
protected:
    void SetUp() override {
        CRC16_Init();
    }
};

TEST_F(CrcTest, StandardCheckValue)
{
    const u8 data[] = "123456789";
    u16 crc = CRC16_Compute(data, 9U);
    EXPECT_EQ(crc, CRC16_CHECK);
}

TEST_F(CrcTest, NullReturnsZero)
{
    u16 crc = CRC16_Compute(NULL, 4U);
    EXPECT_EQ(crc, 0U);
}

TEST_F(CrcTest, ZeroLengthReturnsZero)
{
    const u8 data[] = { 0x01, 0x02 };
    u16 crc = CRC16_Compute(data, 0U);
    EXPECT_EQ(crc, 0U);
}

TEST_F(CrcTest, CorruptionDetected)
{
    u8 data[] = { 0x01, 0x02, 0xAA, 0xBB };
    u16 good = CRC16_Compute(data, sizeof(data));
    data[2] ^= 0xFF;
    u16 bad = CRC16_Compute(data, sizeof(data));
    EXPECT_NE(good, bad);
}

TEST_F(CrcTest, ByteSwapDetected)
{
    u8 original[] = { 0x01, 0x02 };
    u8 swapped[]  = { 0x02, 0x01 };
    u16 crc_orig    = CRC16_Compute(original, 2U);
    u16 crc_swapped = CRC16_Compute(swapped,  2U);
    EXPECT_NE(crc_orig, crc_swapped);
}

TEST_F(CrcTest, ZeroInitVulnerabilityAvoided)
{
    u8 zeros[] = { 0x00, 0x00, 0x00, 0x00 };
    u16 crc = CRC16_Compute(zeros, sizeof(zeros));
    EXPECT_NE(crc, 0x0000U);
}

TEST_F(CrcTest, IncrementalMatchesFull)
{
    u8 data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    u16 full = CRC16_Compute(data, 4U);
    u16 inc  = CRC16_INIT;
    for (u8 i = 0; i < 4U; i++) {
        inc = CRC16_Update(inc, data[i]);
    }
    EXPECT_EQ(full, inc);
}

TEST_F(CrcTest, VerifyPassesOnValidData)
{
    const u8 data[] = "123456789";
    EXPECT_TRUE(CRC16_Verify(data, 9U, CRC16_CHECK));
}

TEST_F(CrcTest, VerifyFailsOnCorruptData)
{
    u8 data[] = "123456789";
    data[0] ^= 0xFF;
    EXPECT_FALSE(CRC16_Verify(data, 9U, CRC16_CHECK));
}
