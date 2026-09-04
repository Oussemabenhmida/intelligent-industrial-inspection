#include "crc.h"

static u16 crc_table[256];
static bool table_ready = false;

void CRC16_Init(void)
{
    for (u16 i = 0; i < 256U; i++) {
        u16 crc = (u16)(i << 8U);

        for (u8 bit = 0; bit < 8U; bit++) {
            if (crc & 0x8000U) {
                crc = (u16)((crc << 1U) ^ CRC16_POLY);
            } else {
                crc = (u16)(crc << 1U);
            }
        }
        crc_table[i] = crc;
    }
    table_ready = true;
}

u16 CRC16_Update(u16 crc, u8 byte)
{
    u8 index = (u8)((crc >> 8U) ^ byte);
    crc = (u16)((crc << 8U) ^ crc_table[index]);
    return crc;
}

u16 CRC16_Compute(const u8 *data, u16 len)
{
    if (data == NULL || len == 0U) return 0U;

    u16 crc = CRC16_INIT;
    for (u16 i = 0; i < len; i++) {
        crc = CRC16_Update(crc, data[i]);
    }
    return crc;
}

bool CRC16_Verify(const u8 *data, u16 len, u16 expected)
{
    return CRC16_Compute(data, len) == expected;
}
