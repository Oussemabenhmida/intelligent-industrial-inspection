# ADR-001: Host-Based FreeRTOS Windows Port

## Status
Accepted

## Context

The project requires a real-time controller that demonstrates FreeRTOS
concepts including task scheduling, queues, mutexes, semaphores, watchdog
supervision, and task timing. The target hardware is an STM32 MCU, but
physical hardware is not yet available during the development phase.

Three options were considered for implementing the real-time controller:

1. Run FreeRTOS on real STM32 hardware
2. Simulate RTOS concepts using POSIX threads on Linux
3. Use the official FreeRTOS Windows/MinGW simulator port

## Decision

Use the official FreeRTOS Windows simulator port (MSVC-MingW) running
on the development PC.

## Alternatives Considered

**Real STM32 hardware**
Would provide the most authentic real-time behavior but requires physical
hardware, an ST-Link programmer, and a hardware bring-up phase before any
software development can begin. Premature for an early development phase.

**POSIX thread simulation**
Would work on Linux but is not an official FreeRTOS port. Behavioral
differences from the real FreeRTOS scheduler would require correction
when porting to hardware later. Also not portable to Windows natively.

**FreeRTOS Windows port**
The official port maintained by the FreeRTOS team. Uses Windows threads
to simulate FreeRTOS task scheduling. Behaviorally close to the real
scheduler. Runs on the same development machine as the rest of the
toolchain. Directly portable to ARM Cortex-M when hardware is available
— only the port layer changes, not the application code.

## Consequences

**Gained:**
- Full FreeRTOS API available immediately without hardware
- Task creation, queues, mutexes, semaphores, notifications all work
- Watchdog supervision and timing behavior can be tested
- Clean migration path to real STM32 hardware — only HAL and port layer change
- Debuggable with GDB on the development PC

**Trade-offs:**
- Windows thread scheduling is not a hard real-time scheduler
- Timing jitter is higher than on a real MCU
- Interrupt-driven behavior cannot be fully simulated
- Stack overflow detection behavior may differ slightly from hardware

## Notes

The FreeRTOS kernel is included as a Git submodule pointing to the
official FreeRTOS-Kernel repository. This ensures the exact kernel
version is reproducible and tracked.
