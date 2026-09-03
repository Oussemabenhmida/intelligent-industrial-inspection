#include <stdio.h>
#include "ring_buffer.h"

static void print_rb(const char *label, RingBuffer *rb)
{
    printf("[%s] count=%u  free=%u  empty=%d  full=%d\n",
           label,
           RB_Count(rb),
           RB_Free(rb),
           RB_IsEmpty(rb),
           RB_IsFull(rb));
}

int main(void)
{
    printf("=== Ring Buffer Smoke Test ===\n");

    RingBuffer rb;
    RB_Init(&rb);

    /* ── Test 1: starts empty ── */
    printf("\n--- Initial state ---\n");
    print_rb("init", &rb);

    /* ── Test 2: write bytes ── */
    printf("\n--- Write 0xAA, 0xBB, 0xCC ---\n");
    printf("Write 0xAA: %s\n", RB_Write(&rb, 0xAA) ? "ok" : "full");
    printf("Write 0xBB: %s\n", RB_Write(&rb, 0xBB) ? "ok" : "full");
    printf("Write 0xCC: %s\n", RB_Write(&rb, 0xCC) ? "ok" : "full");
    print_rb("after 3 writes", &rb);

    /* ── Test 3: read bytes back ── */
    printf("\n--- Read bytes (expect FIFO order) ---\n");
    u8 byte = 0;
    RB_Read(&rb, &byte); printf("Read: 0x%02X (expect 0xAA)\n", byte);
    RB_Read(&rb, &byte); printf("Read: 0x%02X (expect 0xBB)\n", byte);
    RB_Read(&rb, &byte); printf("Read: 0x%02X (expect 0xCC)\n", byte);
    print_rb("after 3 reads", &rb);

    /* ── Test 4: read from empty ── */
    printf("\n--- Read from empty buffer ---\n");
    bool ok = RB_Read(&rb, &byte);
    printf("RB_Read on empty: %s\n", ok ? "WRONG" : "returned false (correct)");

    /* ── Test 5: fill to capacity ── */
    printf("\n--- Fill buffer to capacity (SIZE-1 = %u slots) ---\n",
           RING_BUFFER_SIZE - 1U);
    for (u8 i = 0; i < RING_BUFFER_SIZE - 1U; i++) {
        RB_Write(&rb, i);
    }
    print_rb("full", &rb);

    /* ── Test 6: write to full ── */
    printf("\n--- Write to full buffer ---\n");
    ok = RB_Write(&rb, 0xFF);
    printf("RB_Write on full: %s\n", ok ? "WRONG" : "returned false (correct)");

    /* ── Test 7: wrap-around ── */
    printf("\n--- Wrap-around test ---\n");
    RB_Read(&rb, &byte); printf("Read: 0x%02X (expect 0x00)\n", byte);
    RB_Read(&rb, &byte); printf("Read: 0x%02X (expect 0x01)\n", byte);
    RB_Write(&rb, 0xDE);
    RB_Write(&rb, 0xAD);
    print_rb("after wrap", &rb);

    /* ── Test 8: drain completely ── */
    printf("\n--- Drain buffer ---\n");
    while (!RB_IsEmpty(&rb)) {
        RB_Read(&rb, &byte);
    }
    print_rb("drained", &rb);

    printf("\n=== Ring buffer smoke test complete ===\n");
    return 0;
}
