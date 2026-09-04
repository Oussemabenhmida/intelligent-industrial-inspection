# Intelligent Industrial Edge-AI Inspection and Real-Time Control System

An end-to-end industrial inspection platform combining **Embedded C, C++, FreeRTOS, computer vision, and Edge AI** on an NVIDIA Jetson Nano.

The system captures images of welded components, runs AI inference to detect defects, and transmits results through a validated binary protocol to a real-time controller that produces a final machine decision: **ACCEPT, REJECT, REINSPECT, or FAULT**.

---

## Architecture

    Camera -> OpenCV -> YOLO/TensorRT -> AI Decision
                                              |
                                    Versioned Binary Protocol
                                    CRC-16 / SEQ / Flow Control
                                              |
                                    FreeRTOS Controller
                                    +-- CommunicationTask
                                    +-- InspectionTask
                                    +-- ControlTask
                                    +-- WatchdogTask
                                    +-- DiagnosticsTask
                                    +-- LoggerTask
                                              |
                                    ACCEPT / REJECT / REINSPECT / FAULT
                                              |
                                    Diagnostics / Logging / Dashboard

---

## Repository Structure

    firmware/
      hal/          GPIO, UART, ring buffer, CRC simulation
      rtos/         FreeRTOS kernel (submodule) + config
      application/  Controller tasks, state machine, watchdog

    protocol/       Binary packet serialization/deserialization

    ai/             Dataset, training, evaluation, inference, export
    edge/           Jetson Nano camera, preprocessing, inference

    tests/
      unit/         GoogleTest -- CRC, ring buffer, protocol
      integration/  End-to-end pipeline tests
      fault_injection/ Corrupted packets, timeouts, low confidence

    config/         Centralized system configuration
    docs/           Architecture, memory model, protocol spec, FMEA, ADRs

---

## Embedded C Subsystem

Low-level firmware foundation with simulated MCU peripherals:

- **GPIO** -- register block simulation, MODER/ODR/BSRR/IDR, bit manipulation, read-modify-write
- **UART** -- SR flag polling, TX/RX, timeout, guard checks
- **Ring buffer** -- ISR-safe FIFO, power-of-2 wrap using bitwise AND, full/empty detection
- **CRC-16/CCITT** -- lookup table implementation, corruption and byte-swap detection

All peripheral structures mirror real STM32 register layouts.

---

## Communication Protocol

Custom binary protocol between the Jetson and the controller:

    [SOF][VER][SEQ][MSG_ID][LENGTH][PAYLOAD][CRC-16]

- SOF sync byte, protocol version negotiation
- Sequence numbers for missing/duplicate packet detection
- CRC-16/CCITT integrity validation
- Defined flow control and backpressure policy
- Startup handshake with version exchange

---

## FreeRTOS Controller

Host-based FreeRTOS simulation representing the real-time MCU/ECU layer:

| Task | Responsibility |
|------|---------------|
| CommunicationTask | Receives and validates packets |
| InspectionTask | Applies confidence threshold policy |
| ControlTask | Drives state machine, produces decisions |
| WatchdogTask | Monitors task health, triggers safe state |
| DiagnosticsTask | Reports system health and queue status |
| LoggerTask | Structured log output with timestamps |

Tasks communicate exclusively through FreeRTOS queues. Shared state protected by mutex.

---

## AI / Edge Pipeline (Jetson Nano)

- **Dataset** -- industrial weld defect images, documented class distribution and split strategy
- **Training** -- PyTorch / Ultralytics YOLO
- **Evaluation** -- precision, recall, F1, confusion matrix, confidence calibration
- **Deployment** -- ONNX export, TensorRT optimization, FP16 inference on Jetson GPU
- **Decision policy** -- confidence threshold selected from precision-recall curve
- **Cost model** -- explicit false-positive / false-negative cost analysis

---

## Testing

- **25 unit tests** (GoogleTest) covering CRC, ring buffer, and protocol layer
- **Integration tests** -- full pipeline from packet to decision
- **Fault injection** -- corrupted CRC, bad SOF, version mismatch, low confidence, timeout
- **Regression tests** -- golden dataset for model versioning

---

## Technology Stack

| Layer | Technologies |
|-------|-------------|
| Embedded | C11, C++17, FreeRTOS, CMake, GCC, GDB |
| AI/CV | Python, PyTorch, Ultralytics YOLO, OpenCV, ONNX, TensorRT |
| Edge | NVIDIA Jetson Nano, CUDA, JetPack |
| Testing | GoogleTest, AddressSanitizer, UBSan |
| Tools | Git, CMake, GDB, sanitizers |

---

## Build

    git clone --recurse-submodules <repo-url>
    cd intelligent-industrial-inspection
    mkdir build && cd build
    cmake .. -G "MinGW Makefiles"
    mingw32-make

Run unit tests:

    ./tests/unit/unit_tests.exe

Run the controller simulation:

    ./firmware/application/controller.exe

---

## Project Goals

This project demonstrates practical engineering across multiple layers:

- Low-level embedded C -- memory, pointers, registers, drivers, buffers, protocols
- Embedded C++ -- architecture, abstractions, state machines, resource management
- Real-time systems -- FreeRTOS scheduling, synchronization, timing, watchdogs
- AI/ML -- dataset engineering, training, evaluation, calibration, deployment
- Edge AI -- Jetson Nano, TensorRT, performance optimization
- Industrial software -- diagnostics, fault handling, FMEA, regression testing
