# ADR-005: Power-of-2 Ring Buffer with Bitwise AND Index Wrapping

## Status
Accepted

## Context

The UART receive path requires a buffer that can be written by an ISR
and read by the application without blocking either side. The buffer
implementation must be efficient enough for use in an ISR context where
execution time must be minimal.

## Decision

Implement a ring buffer with the following properties:

- Fixed size of 64 bytes (power of 2)
- Index wrapping using bitwise AND: index = (index + 1) & (SIZE - 1)
- Separate head (write) and tail (read) indices
- One slot sacrificed to distinguish full from empty without a counter
- No dynamic memory allocation

## Alternatives Considered

**Modulo wrapping: index = (index + 1) % SIZE**
Works for any buffer size. However, the modulo operation requires
integer division, which on microcontrollers without a hardware divider
can take 20-100 clock cycles. In an ISR that may fire hundreds of times
per second, this overhead accumulates. Bitwise AND achieves the same
wrapping in a single cycle.

**Separate count variable**
Using a separate counter to track occupancy avoids sacrificing one slot.
However, maintaining a counter requires an additional read-modify-write
in both the write and read paths, increasing ISR execution time and
creating a potential race condition if the counter is not protected
atomically.

**Dynamic sizing**
Configurable buffer size at runtime would be more flexible but would
require either modulo wrapping (slow) or runtime verification that the
size is a power of 2 (adds complexity). The 64-byte fixed size is
sufficient for the protocol packet sizes used in this project.

**Power-of-2 with bitwise AND**
SIZE - 1 produces a bitmask that covers exactly the valid index range.
Bitwise AND with this mask is a single CPU instruction. Works correctly
for any power-of-2 size. The compile-time check ensures the constraint
is enforced:

    typedef char rb_size_check_[
        ((RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1U)) == 0U) ? 1 : -1];

## Consequences

**Gained:**
- Single-cycle index wrapping — minimal ISR overhead
- No division instruction required
- Compile-time enforcement of power-of-2 constraint
- Simple, predictable memory layout
- ISR-safe when head is written only by producer and tail only by consumer

**Trade-offs:**
- Buffer size constrained to powers of 2 (32, 64, 128, 256...)
- One slot wasted to distinguish full from empty
- Maximum capacity is SIZE - 1 bytes, not SIZE bytes
