# Failure Mode and Effects Analysis (FMEA)

## Scope

This FMEA covers the Intelligent Industrial Inspection ECU system,
including the Edge-AI subsystem (Jetson Nano), the communication
protocol, and the FreeRTOS real-time controller.

The objective is not formal safety certification but structured
analysis of failure modes and their handling.

Severity levels:
- **CRITICAL** — system cannot produce a valid inspection decision
- **HIGH**     — inspection decision may be incorrect or delayed
- **MEDIUM**   — system continues but with degraded capability
- **LOW**      — minor impact, system continues normally

---

## 1. Edge-AI Subsystem Failures

| ID | Component | Failure Mode | Effect | Severity | Detection | Response |
|----|-----------|-------------|--------|----------|-----------|----------|
| F-01 | Camera | Camera unavailable or timeout | No frame available for inspection | CRITICAL | Camera read returns None / timeout | Pipeline returns NO_DETECTION result → controller receives INSPECT_INVALID → FAULT state |
| F-02 | Camera | Blurred or overexposed image | YOLO inference on low-quality input | HIGH | Image quality check (blur, brightness) | Frame rejected before inference, reinspect requested |
| F-03 | YOLO model | Inference timeout | No detection result | CRITICAL | Inference timer | Pipeline returns NO_DETECTION → FAULT |
| F-04 | YOLO model | False positive detection | Good weld classified as defect | HIGH | Confidence threshold, MobileNetV2 fusion | Conservative fusion — YOLO defect cannot be downgraded but low-confidence results trigger REINSPECT |
| F-05 | YOLO model | False negative detection | Defect weld classified as good | CRITICAL | Autoencoder anomaly score | High anomaly score on GOOD_WELD triggers additional scrutiny |
| F-06 | MobileNetV2 | Model load failure | Classifier unavailable | MEDIUM | Model load exception at startup | Pipeline continues with YOLO-only decision, classifier_enabled flag set false |
| F-07 | Autoencoder | Reconstruction error unstable | Anomaly score unreliable | MEDIUM | Score outside calibrated range | Score flagged as unreliable, XGBoost prediction proceeds without anomaly feature |
| F-08 | XGBoost | Force prediction failure | No peel force estimate | MEDIUM | Predictor exception | Decision falls back to visual class only, force marked as None |
| F-09 | Jetson Nano | System overload / OOM | Inference skipped or delayed | HIGH | Pipeline health counters, latency monitor | Consecutive error counter triggers pipeline health warning |
| F-10 | Jetson Nano | GPU unavailable | Inference runs on CPU | MEDIUM | CUDA device check at startup | System falls back to CPU inference, latency increases |

---

## 2. Communication Protocol Failures

| ID | Component | Failure Mode | Effect | Severity | Detection | Response |
|----|-----------|-------------|--------|----------|-----------|----------|
| F-11 | TCP link | Connection refused | Controller unreachable | HIGH | Socket connect exception | ControllerSender logs warning, returns False, pipeline continues without controller |
| F-12 | TCP link | Connection dropped mid-session | Packets lost | HIGH | Socket send exception | ControllerSender sets socket to None, reconnects on next send attempt |
| F-13 | Packet | CRC error | Corrupted packet received | HIGH | CRC mismatch in PROTO_Deserialize | Packet dropped, PARSE_BAD_CRC logged, no decision produced for that packet |
| F-14 | Packet | Bad SOF byte | Frame sync lost | MEDIUM | SOF check in PROTO_Deserialize | Packet dropped, PARSE_BAD_SOF logged |
| F-15 | Packet | Protocol version mismatch | Incompatible sender/receiver | HIGH | VER check in PROTO_Deserialize | Packet dropped, PARSE_BAD_VERSION logged, operator must align versions |
| F-16 | Packet | Missing packet (SEQ gap) | Inspection result lost | HIGH | SEQ check in TCPListenerTask | Gap logged as warning, system continues with next valid packet |
| F-17 | Packet | Duplicate packet (SEQ repeat) | Same result processed twice | MEDIUM | SEQ check in TCPListenerTask | Duplicate logged, duplicate decision produced (acceptable for idempotent decisions) |
| F-18 | Packet | Oversized payload | Buffer overflow attempt | CRITICAL | Length check before recv | Packet rejected, connection closed, listener waits for new connection |

---

## 3. FreeRTOS Controller Failures

| ID | Component | Failure Mode | Effect | Severity | Detection | Response |
|----|-----------|-------------|--------|----------|-----------|----------|
| F-19 | CommunicationTask | Task hangs | No packets received | CRITICAL | WatchdogTask timeout | WATCHDOG_TIMEOUT_MS exceeded → gSystemState = FAULT |
| F-20 | InspectionTask | Task hangs | Decisions not produced | CRITICAL | WatchdogTask timeout | gSystemState = FAULT |
| F-21 | ControlTask | Task hangs | No machine decisions | CRITICAL | WatchdogTask timeout | gSystemState = FAULT |
| F-22 | DiagnosticsTask | Task hangs | No health reporting | LOW | WatchdogTask timeout | FAULT flagged but system may continue if other tasks healthy |
| F-23 | LoggerTask | Log queue full | Log messages dropped | LOW | Queue high-water mark | Oldest log entries dropped (lower priority than inspection) |
| F-24 | InspectionQueue | Queue full | Inspection results dropped | HIGH | xQueueSend returns pdFALSE | Warning logged, packet dropped, backpressure policy applied |
| F-25 | DecisionQueue | Queue full | Decisions dropped | HIGH | xQueueSend returns pdFALSE | Warning logged |
| F-26 | Mutex | Mutex deadlock | System state inaccessible | CRITICAL | WatchdogTask timeout on blocked tasks | All mutex acquisitions use timeout — pdMS_TO_TICKS(100) maximum wait |
| F-27 | Heap | Malloc failure | Task or queue creation fails | CRITICAL | vApplicationMallocFailedHook | System halts with diagnostic message |
| F-28 | Stack | Stack overflow | Task corruption | CRITICAL | configCHECK_FOR_STACK_OVERFLOW=2 | vApplicationStackOverflowHook called, system halts |

---

## 4. System-Level Failures

| ID | Component | Failure Mode | Effect | Severity | Detection | Response |
|----|-----------|-------------|--------|----------|-----------|----------|
| F-29 | AI confidence | Low confidence result | Uncertain classification | MEDIUM | Confidence < CONFIDENCE_THRESHOLD | DECISION_REINSPECT produced, part held for second inspection |
| F-30 | AI confidence | All results low confidence | Systematic model degradation | HIGH | Persistent REINSPECT pattern | Operator alert via dashboard, model drift investigation |
| F-31 | State machine | Invalid state transition | Undefined system behavior | HIGH | Explicit state validation in ControlTask | Transition rejected, current state preserved, warning logged |
| F-32 | System | Jetson boot delay | Controller starts before AI ready | MEDIUM | Startup handshake timeout | Controller remains in IDLE state, TCPListener waits for connection |
| F-33 | System | Power loss during inspection | Incomplete inspection result | HIGH | Sudden absence of heartbeat | Part flagged as uninspected, requires manual review |

---

## 5. Summary Statistics

| Severity | Count |
|----------|-------|
| CRITICAL | 13 |
| HIGH     | 13 |
| MEDIUM   | 6 |
| LOW      | 3 |
| **Total**| **35** |

---

## 6. Key Risk Mitigations

**CRC validation** — all packets validated before processing (F-13)

**Watchdog supervision** — all tasks monitored with kick timeout (F-19 to F-23)

**Mutex timeouts** — no mutex acquisition blocks indefinitely (F-26)

**Stack overflow detection** — FreeRTOS level 2 stack checking enabled (F-28)

**Conservative AI fusion** — YOLO defect cannot be downgraded by classifier (F-04)

**Confidence threshold** — low confidence results trigger REINSPECT not ACCEPT (F-29)

**Non-blocking sender** — controller unreachable does not block AI pipeline (F-11)

**Queue backpressure** — defined behavior for all queue full conditions (F-24, F-25)
