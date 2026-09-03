#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "hal_types.h"

/* ─────────────────────────────────────────────
   Ring Buffer

   A fixed-size circular buffer for byte data.
   Designed for use between an ISR producer
   and an application consumer.

   Size must be a power of 2 to allow efficient
   index wrapping using bitwise AND instead of
   modulo division.
───────────────────────────────────────────── */

#define RING_BUFFER_SIZE  64U   /* Must be a power of 2 */

/* Compile-time check that size is power of 2 */
typedef char rb_size_check_[(
    (RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1U)) == 0U) ? 1 : -1];

typedef struct {
    u8   buffer[RING_BUFFER_SIZE];  /* Storage array          */
    volatile u16 head;              /* Write index (producer) */
    volatile u16 tail;              /* Read index  (consumer) */
} RingBuffer;

/* ─────────────────────────────────────────────
   API
───────────────────────────────────────────── */

void     RB_Init(RingBuffer *rb);
bool     RB_Write(RingBuffer *rb, u8 byte);
bool     RB_Read(RingBuffer *rb, u8 *byte);
bool     RB_IsEmpty(const RingBuffer *rb);
bool     RB_IsFull(const RingBuffer *rb);
u16      RB_Count(const RingBuffer *rb);
u16      RB_Free(const RingBuffer *rb);

#endif /* RING_BUFFER_H */
