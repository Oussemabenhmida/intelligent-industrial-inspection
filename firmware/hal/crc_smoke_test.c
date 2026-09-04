#include <stdio.h>
#include <string.h>
#include "crc.h"

int main(void)
{
    printf("=== CRC-16/CCITT Smoke Test ===\n");

    CRC16_Init();

    /* ── Test 1: standard check value ── */
    printf("\n--- Standard check value ---\n");
    const u8 check_str[] = "123456789";
    u16 crc = CRC16_Compute(check_str, 9U);
    printf("CRC(\"123456789\") = 0x%04X (expect 0x29B1): %s\n",
           crc, crc == CRC16_CHECK ? "PASS" : "FAIL");

    /* ── Test 2: verify function ── */
    printf("\n--- Verify function ---\n");
    bool ok = CRC16_Verify(check_str, 9U, CRC16_CHECK);
    printf("CRC16_Verify: %s\n", ok ? "PASS" : "FAIL");

    /* ── Test 3: single byte corruption detected ── */
    printf("\n--- Corruption detection ---\n");
    u8 packet[] = { 0x01, 0x02, 0xAA, 0xBB, 0x03 };
    u16 good_crc = CRC16_Compute(packet, sizeof(packet));
    printf("Good CRC: 0x%04X\n", good_crc);
    packet[2] = 0xAB;
    u16 bad_crc = CRC16_Compute(packet, sizeof(packet));
    printf("Corrupt CRC: 0x%04X\n", bad_crc);
    printf("Corruption detected: %s\n", good_crc != bad_crc ? "YES (correct)" : "NO (wrong)");

    /* ── Test 4: byte swap detected ── */
    printf("\n--- Byte swap detection ---\n");
    u8 original[] = { 0x01, 0x02 };
    u8 swapped[]  = { 0x02, 0x01 };
    u16 crc_orig    = CRC16_Compute(original, 2U);
    u16 crc_swapped = CRC16_Compute(swapped,  2U);
    printf("CRC {0x01,0x02} = 0x%04X\n", crc_orig);
    printf("CRC {0x02,0x01} = 0x%04X\n", crc_swapped);
    printf("Swap detected: %s\n", crc_orig != crc_swapped ? "YES (correct)" : "NO (wrong)");

    /* ── Test 5: zero-init vulnerability avoided ── */
    printf("\n--- Zero-init vulnerability check ---\n");
    u8 zeros[] = { 0x00, 0x00, 0x00, 0x00 };
    u16 zero_crc = CRC16_Compute(zeros, sizeof(zeros));
    printf("CRC of all zeros = 0x%04X (must not be 0x0000): %s\n",
           zero_crc, zero_crc != 0x0000U ? "PASS" : "FAIL");

    /* ── Test 6: incremental update matches full compute ── */
    printf("\n--- Incremental update ---\n");
    u8 data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    u16 full = CRC16_Compute(data, 4U);
    u16 inc  = CRC16_INIT;
    for (u8 i = 0; i < 4U; i++) {
        inc = CRC16_Update(inc, data[i]);
    }
    printf("Full compute : 0x%04X\n", full);
    printf("Incremental  : 0x%04X\n", inc);
    printf("Match: %s\n", full == inc ? "PASS" : "FAIL");

    /* ── Test 7: NULL guard ── */
    printf("\n--- NULL guard ---\n");
    u16 null_crc = CRC16_Compute(NULL, 4U);
    printf("CRC16_Compute(NULL) = 0x%04X (expect 0x0000): %s\n",
           null_crc, null_crc == 0U ? "PASS" : "FAIL");

    printf("\n=== CRC smoke test complete ===\n");
    return 0;
}
