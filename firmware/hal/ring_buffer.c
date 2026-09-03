#include "ring_buffer.h"

/* ─────────────────────────────────────────────
   Index wrap using bitwise AND.

   Because RING_BUFFER_SIZE is a power of 2,
   (index & (SIZE - 1)) is identical to
   (index % SIZE) but much faster — no division.

   Example with SIZE=64:
   index 64 → 64 & 63 = 0  (wraps)
   index 65 → 65 & 63 = 1
───────────────────────────────────────────── */

#define RB_MASK  (RING_BUFFER_SIZE - 1U)

/* ─────────────────────────────────────────────
   RB_Init — zero the buffer and reset indices
───────────────────────────────────────────── */

void RB_Init(RingBuffer *rb)
{
    if (rb == NULL) return;
    rb->head = 0;
    rb->tail = 0;
    for (u16 i = 0; i < RING_BUFFER_SIZE; i++) {
        rb->buffer[i] = 0;
    }
}

/* ─────────────────────────────────────────────
   RB_IsEmpty

   Empty when head == tail.
   Both indices start at 0 and advance together
   as bytes are written and read.
───────────────────────────────────────────── */

bool RB_IsEmpty(const RingBuffer *rb)
{
    if (rb == NULL) return true;
    return rb->head == rb->tail;
}

/* ─────────────────────────────────────────────
   RB_IsFull

   Full when the next write position would
   equal tail — we sacrifice one slot to
   distinguish full from empty without a
   separate counter.
───────────────────────────────────────────── */

bool RB_IsFull(const RingBuffer *rb)
{
    if (rb == NULL) return true;
    return ((rb->head + 1U) & RB_MASK) == rb->tail;
}

/* ─────────────────────────────────────────────
   RB_Count — bytes currently in the buffer
───────────────────────────────────────────── */

u16 RB_Count(const RingBuffer *rb)
{
    if (rb == NULL) return 0;
    return (rb->head - rb->tail) & RB_MASK;
}

/* ─────────────────────────────────────────────
   RB_Free — available space remaining
───────────────────────────────────────────── */

u16 RB_Free(const RingBuffer *rb)
{
    if (rb == NULL) return 0;
    return (RING_BUFFER_SIZE - 1U) - RB_Count(rb);
}

/* ─────────────────────────────────────────────
   RB_Write — called from ISR or producer.
   Returns false if buffer is full.
   Does NOT block.
───────────────────────────────────────────── */

bool RB_Write(RingBuffer *rb, u8 byte)
{
    if (rb == NULL || RB_IsFull(rb)) return false;

    rb->buffer[rb->head & RB_MASK] = byte;
    rb->head = (rb->head + 1U) & RB_MASK;

    return true;
}

/* ─────────────────────────────────────────────
   RB_Read — called from application/task.
   Returns false if buffer is empty.
   Does NOT block.
───────────────────────────────────────────── */

bool RB_Read(RingBuffer *rb, u8 *byte)
{
    if (rb == NULL || byte == NULL || RB_IsEmpty(rb)) return false;

    *byte = rb->buffer[rb->tail & RB_MASK];
    rb->tail = (rb->tail + 1U) & RB_MASK;

    return true;
}
