# Memory Model

## Overview

This document describes the memory layout and resource consumption of
the FreeRTOS controller. Values are measured from the live system using
FreeRTOS runtime APIs after 10 seconds of normal operation.

---

## Heap Configuration

    configTOTAL_HEAP_SIZE : 65536 bytes (64 KB)
    Heap allocator        : heap_4.c (allocation + coalescence)

### Measured Heap Usage

    Heap total  : 65536 bytes
    Heap free   : 32848 bytes
    Heap used   : 32688 bytes
    Utilization : 49.9%
    Margin      : 32848 bytes free (50.1%)

### Heap Allocation Breakdown (estimated)

    Object                           Approx size
    TCB per task (x7 tasks)          7 x 200  = 1400 bytes
    Task stacks (all tasks)                    = 13312 bytes
    InspectionQueue (8 x 16 bytes)             = 128 bytes + overhead
    DecisionQueue   (8 x  4 bytes)             = 32  bytes + overhead
    LogQueue       (16 x 72 bytes)             = 1152 bytes + overhead
    StateMutex                                 = 80  bytes
    Timer service overhead                     = 500 bytes
    Alignment padding and port overhead        = remainder
    Measured total                             = 32688 bytes

---

## Task Stack Usage

Stack sizes are configured in controller.h in words (4 bytes each).
High-water mark (HWM) is the minimum free words ever recorded.

    Task           Config(W)  Config(B)  HWM(W)  HWM(B)  Used(W)
    TCPListen      1024       4096       1021    4084    3
    InspTask        512       2048        509    2036    3
    CtrlTask        512       2048        509    2036    3
    Tmr Svc         512       2048        509    2036    3
    Watchdog        256       1024        253    1012    3
    Diag            256       1024        253    1012    3
    Logger          256       1024        253    1012    3
    IDLE            256       1024        253    1012    3

    Total configured : 3584 words = 14336 bytes

### Why HWM Is Low in the Windows Simulation

All tasks show only 3 words used. This is expected because the Windows
port uses native Windows threads. The FreeRTOS stack array is used only
for TCB bookkeeping, not for actual task execution. On a real Cortex-M
target, local variables and function call frames consume the FreeRTOS
stack, so usage would be significantly higher.

### Stack Sizing for Real STM32 Hardware (Recommended)

    Task               Recommended(W)  Reason
    CommunicationTask  256             simple queue operations
    InspectionTask     512             snprintf + queue + logic
    ControlTask        512             snprintf + mutex + switch
    WatchdogTask       256             simple loop + LOG_Send
    DiagnosticsTask    512             uxTaskGetSystemState call
    LoggerTask         256             printf + queue read
    IDLE               128             FreeRTOS minimum

Validate on real hardware using uxTaskGetStackHighWaterMark().
Increase any task whose margin drops below 20 percent.

---

## Queue Memory

    Queue               Depth  Item(B)  Total(B)
    xInspectionQueue    8      16       128 + overhead
    xDecisionQueue      8       4        32 + overhead
    xLogQueue           16     72      1152 + overhead

Depth rationale:

    InspectionQueue (8)  holds 4 seconds at 2Hz — buffers ControlTask bursts
    DecisionQueue   (8)  decisions consumed faster than produced
    LogQueue       (16)  deeper to prevent loss during high-activity periods

---

## Memory Policy

The system follows a static allocation preference:

1. All task stacks allocated at scheduler start — no runtime growth
2. All queues allocated at scheduler start — fixed depth, no resize
3. No malloc or free calls in steady-state operation
4. heap_4 supports deallocation if needed for future features
5. vApplicationMallocFailedHook halts if heap is exhausted

This ensures predictable memory behavior and eliminates fragmentation.

---

## Worst-Case Budget for Real STM32 Target

    Region                           Size(B)
    FreeRTOS heap (tasks + queues)   16384
    Static globals and BSS             512
    Ring buffer UART RX                 64
    CRC lookup table                   512
    Protocol packet buffer              80
    Log message buffer                  72
    Total estimated                  17624

    Recommended minimum RAM : 32 KB
    Comfortable RAM         : 64 KB

An STM32F401RE (96KB RAM) or STM32F446RE (128KB RAM) provides
comfortable margin for this firmware.
