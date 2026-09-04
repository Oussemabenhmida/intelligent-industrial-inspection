#ifndef CRC_H
#define CRC_H

#include "hal_types.h"

#define CRC16_POLY    0x1021U
#define CRC16_INIT    0xFFFFU
#define CRC16_CHECK   0x29B1U

void   CRC16_Init(void);
u16    CRC16_Compute(const u8 *data, u16 len);
u16    CRC16_Update(u16 crc, u8 byte);
bool   CRC16_Verify(const u8 *data, u16 len, u16 expected);

#endif /* CRC_H */
