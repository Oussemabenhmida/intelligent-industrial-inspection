#include <gtest/gtest.h>

extern "C" {
#include "firmware/hal/ring_buffer.h"
}

class RingBufferTest : public ::testing::Test {
protected:
    RingBuffer rb;

    void SetUp() override {
        RB_Init(&rb);
    }
};

TEST_F(RingBufferTest, InitEmpty)
{
    EXPECT_TRUE(RB_IsEmpty(&rb));
    EXPECT_FALSE(RB_IsFull(&rb));
    EXPECT_EQ(RB_Count(&rb), 0U);
    EXPECT_EQ(RB_Free(&rb), RING_BUFFER_SIZE - 1U);
}

TEST_F(RingBufferTest, WriteAndRead)
{
    EXPECT_TRUE(RB_Write(&rb, 0xAA));
    EXPECT_EQ(RB_Count(&rb), 1U);

    u8 byte = 0;
    EXPECT_TRUE(RB_Read(&rb, &byte));
    EXPECT_EQ(byte, 0xAAU);
    EXPECT_TRUE(RB_IsEmpty(&rb));
}

TEST_F(RingBufferTest, FifoOrder)
{
    RB_Write(&rb, 0x01);
    RB_Write(&rb, 0x02);
    RB_Write(&rb, 0x03);

    u8 byte = 0;
    RB_Read(&rb, &byte); EXPECT_EQ(byte, 0x01U);
    RB_Read(&rb, &byte); EXPECT_EQ(byte, 0x02U);
    RB_Read(&rb, &byte); EXPECT_EQ(byte, 0x03U);
}

TEST_F(RingBufferTest, ReadFromEmptyReturnsFalse)
{
    u8 byte = 0;
    EXPECT_FALSE(RB_Read(&rb, &byte));
}

TEST_F(RingBufferTest, WriteToFullReturnsFalse)
{
    for (u8 i = 0; i < RING_BUFFER_SIZE - 1U; i++) {
        EXPECT_TRUE(RB_Write(&rb, i));
    }
    EXPECT_TRUE(RB_IsFull(&rb));
    EXPECT_FALSE(RB_Write(&rb, 0xFF));
}

TEST_F(RingBufferTest, WrapAround)
{
    for (u8 i = 0; i < RING_BUFFER_SIZE - 1U; i++) {
        RB_Write(&rb, i);
    }
    u8 byte = 0;
    RB_Read(&rb, &byte); EXPECT_EQ(byte, 0x00U);
    RB_Read(&rb, &byte); EXPECT_EQ(byte, 0x01U);

    EXPECT_TRUE(RB_Write(&rb, 0xDE));
    EXPECT_TRUE(RB_Write(&rb, 0xAD));
    EXPECT_TRUE(RB_IsFull(&rb));
}

TEST_F(RingBufferTest, CountAndFreeConsistent)
{
    RB_Write(&rb, 0xAA);
    RB_Write(&rb, 0xBB);
    RB_Write(&rb, 0xCC);

    EXPECT_EQ(RB_Count(&rb), 3U);
    EXPECT_EQ(RB_Free(&rb), RING_BUFFER_SIZE - 1U - 3U);
    EXPECT_EQ(RB_Count(&rb) + RB_Free(&rb), RING_BUFFER_SIZE - 1U);
}

TEST_F(RingBufferTest, NullSafety)
{
    EXPECT_TRUE(RB_IsEmpty(NULL));
    EXPECT_TRUE(RB_IsFull(NULL));
    EXPECT_EQ(RB_Count(NULL), 0U);
    EXPECT_FALSE(RB_Write(NULL, 0xAA));
    u8 byte = 0;
    EXPECT_FALSE(RB_Read(NULL, &byte));
}
