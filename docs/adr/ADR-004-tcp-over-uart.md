# ADR-004: TCP Socket Instead of Physical UART for Jetson-Controller Link

## Status
Accepted

## Context

The binary protocol must be transported between the Jetson Nano AI
subsystem and the FreeRTOS controller. A transport mechanism must be
chosen that works in the current development environment.

## Decision

Use a TCP socket connection. The Jetson sender connects to the
controller's TCP listener on port 5000. The binary protocol frame
format is identical to what would be sent over UART — only the
transport layer changes.

## Alternatives Considered

**Physical UART over RS485**
The natural choice for a real industrial deployment. RS485 differential
signaling provides noise immunity and supports cable runs up to 1200m.
However, the Jetson Nano does not expose a spare UART port that is
easily accessible without hardware modification. The existing RS485
interface is already used for Modbus telemetry reading from the
welding machine.

**USB-to-UART adapter**
Would provide a UART interface on the Jetson via USB. Adds a hardware
dependency, requires driver installation on Jetson, and introduces
latency from USB protocol overhead. Not appropriate for a clean
embedded architecture demonstration.

**Shared RS485 port with arbitration**
Sharing the existing RS485 port between Modbus telemetry and the
inspection protocol would require bus arbitration logic and careful
timing coordination. Adds complexity with no benefit over TCP in the
current phase.

**TCP socket**
The Jetson Nano runs Linux and has a network interface. The FreeRTOS
Windows simulator also has network access. TCP provides reliable,
ordered delivery. The binary protocol frame format is transport-
agnostic — the same serialization and CRC logic applies regardless
of whether bytes flow over UART or TCP. Switching to physical UART
later requires changing only the transport layer, not the protocol
or application logic.

## Consequences

**Gained:**
- No hardware dependency during development
- Works immediately on both Jetson and Windows simulator
- Transport-agnostic protocol design means UART migration is clean
- TCP provides reliable delivery — no need to implement retransmit logic
  at the application layer during development

**Trade-offs:**
- TCP is not a real-time transport — jitter is higher than UART
- TCP connection setup adds latency compared to direct UART
- In production, RS485/UART would be preferred for determinism
  and noise immunity

## Migration Path

To migrate to physical UART:
1. Replace TCP socket calls in tcp_listener.c with UART HAL calls
2. Replace TCP socket calls in controller_sender.py with pyserial calls
3. Protocol, CRC, and all application logic remain unchanged
