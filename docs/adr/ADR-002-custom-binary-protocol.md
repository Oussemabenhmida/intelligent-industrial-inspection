# ADR-002: Custom Binary Protocol Instead of JSON or MQTT

## Status
Accepted

## Context

The Jetson Nano AI subsystem must transmit inspection results to the
real-time controller. A communication protocol must be chosen that
works reliably in an industrial environment and is compatible with
the embedded controller's constraints.

## Decision

Use a custom binary protocol with the following frame format:

    [SOF][VER][SEQ][MSG_ID][LENGTH_LO][LENGTH_HI][PAYLOAD...][CRC_LO][CRC_HI]

## Alternatives Considered

**JSON over TCP**
Human-readable and easy to debug. However, JSON parsing on an embedded
controller requires dynamic memory allocation and a parser library.
Variable message length without explicit framing makes synchronization
after corruption difficult. No built-in integrity checking.

**MQTT**
A publish-subscribe protocol designed for IoT. Requires a broker,
adding infrastructure complexity. The broker becomes a single point
of failure. Overhead is significant for a direct point-to-point link
between two devices on the same network.

**Modbus RTU**
Already present in the PFE project for RS485 telemetry reading.
However, Modbus is designed for register-based polling of sensor data,
not for streaming structured inspection results with variable payloads.
Mapping inspection results into Modbus registers would be awkward.

**Custom binary protocol**
Minimal overhead — a 15-byte inspection packet versus 80-120 bytes
for equivalent JSON. Fixed-width fields make parsing deterministic
and stack-allocatable. CRC-16 provides integrity validation. SOF byte
provides frame synchronization. SEQ number detects missing or duplicate
packets. VER field enables protocol versioning. No external dependencies.

## Consequences

**Gained:**
- Minimal wire overhead
- Deterministic parsing with no dynamic allocation
- Built-in integrity checking via CRC-16
- Frame synchronization via SOF byte
- Missing packet detection via sequence numbers
- Protocol version negotiation via VER field
- Full control over evolution of the protocol

**Trade-offs:**
- Not human-readable — requires tooling to inspect raw bytes
- Both sides must implement the same serialization/deserialization
- Extending the protocol requires updating both Jetson sender and controller parser
