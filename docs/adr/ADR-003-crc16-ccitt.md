# ADR-003: CRC-16/CCITT with Precomputed Lookup Table

## Status
Accepted

## Context

The communication protocol requires an integrity check to detect data
corruption in transit. A checksum algorithm must be chosen that provides
adequate error detection with acceptable computational cost on a
resource-constrained embedded controller.

## Decision

Use CRC-16/CCITT-FALSE with the following parameters:

    Polynomial : 0x1021
    Initial    : 0xFFFF
    Input rev  : false
    Output rev : false
    Check value: 0x29B1 (CRC of ASCII "123456789")

Implement using a 512-byte precomputed lookup table for performance.

## Alternatives Considered

**Simple additive checksum**
XOR or sum of all bytes. Fast but weak — cannot detect byte swaps,
certain burst errors, or corrupted zero-byte regions. Insufficient
for industrial use.

**CRC-8**
8-bit output. Hamming distance too low for packets longer than a few
bytes. High probability of undetected errors for the 15-byte packets
used in this protocol.

**CRC-32**
Better error detection than CRC-16 but doubles the CRC field overhead
from 2 bytes to 4 bytes per packet, and requires more computation.
The error detection improvement is not necessary for the packet sizes
used in this protocol.

**CRC-16/CCITT**
Industry standard used in HDLC, X.25, XMODEM, and SD card protocols.
Well-tested. Detects all single-bit errors, all double-bit errors,
all odd numbers of errors, all burst errors of length 16 or less.
Adequate for the packet sizes in this protocol.

**Bit-by-bit vs lookup table implementation**
Bit-by-bit processes each bit individually — 8 iterations per byte.
Lookup table precomputes all 256 possible byte values at initialization,
reducing per-byte computation to one XOR and one table lookup.
The 512-byte table cost is acceptable given the available RAM.

**Initialization value 0xFFFF vs 0x0000**
Starting at 0x0000 would produce CRC=0 for an all-zero message,
making it impossible to distinguish a corrupted zeroed buffer from
a legitimate empty message. Starting at 0xFFFF avoids this
vulnerability and ensures leading zero bytes affect the CRC.

## Consequences

**Gained:**
- Standard, well-understood algorithm with known error detection properties
- Fast computation via lookup table — single table access per byte
- Non-zero CRC for all-zero payloads
- Verifiable against published check value 0x29B1

**Trade-offs:**
- 512 bytes of RAM consumed by lookup table
- Table must be initialized before first use (CRC16_Init call)
- Not cryptographically secure — cannot detect deliberate tampering
