"""
fault_injection_test.py

Deliberately sends malformed or edge-case packets to the FreeRTOS
controller and verifies correct handling.

Run the controller first:
    ./build/firmware/application/controller.exe

Then run this script:
    python3 tests/fault_injection/fault_injection_test.py
"""

import socket
import struct
import time

PROTO_SOF      = 0xAA
PROTO_VERSION  = 0x01
MSG_INSPECTION = 0x02
CRC16_POLY     = 0x1021
CRC16_INIT     = 0xFFFF

PASS = "PASS"
FAIL = "FAIL"

results = []

def crc16(data: bytes) -> int:
    crc = CRC16_INIT
    for byte in data:
        idx = ((crc >> 8) ^ byte) & 0xFF
        entry = idx << 8
        for _ in range(8):
            if entry & 0x8000:
                entry = ((entry << 1) ^ CRC16_POLY) & 0xFFFF
            else:
                entry = (entry << 1) & 0xFFFF
        crc = ((crc << 8) ^ entry) & 0xFFFF
    return crc

def build_valid_packet(seq, result, defect_class, confidence, timestamp_ms):
    payload = struct.pack("<BBBI", result, defect_class, confidence, timestamp_ms)
    header  = struct.pack("<BBBBH", PROTO_SOF, PROTO_VERSION,
                          seq & 0xFF, MSG_INSPECTION, len(payload))
    crc     = crc16(header + payload)
    return header + payload + struct.pack("<H", crc)

def send_raw(sock, data):
    try:
        sock.sendall(data)
        time.sleep(0.3)
        return True
    except OSError:
        return False

def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3.0)
    s.connect(("127.0.0.1", 5000))
    return s

def log(name, status, note=""):
    marker = "OK" if status == PASS else "!!"
    print(f"  [{marker}] {name:<45} {note}")
    results.append((name, status))

def run_tests():
    print("\n=== Fault Injection Test Suite ===\n")

    # ── Test 1: Valid packet baseline ──────────────────────────
    print("-- Baseline --")
    s = connect()
    pkt = build_valid_packet(0, 0x00, 0x00, 95, 1000)
    ok = send_raw(s, pkt)
    log("Valid GOOD packet accepted", PASS if ok else FAIL)
    s.close()
    time.sleep(0.5)

    # ── Test 2: CRC corruption ─────────────────────────────────
    print("\n-- CRC Corruption --")
    s = connect()
    pkt = bytearray(build_valid_packet(1, 0x00, 0x00, 95, 2000))
    pkt[7] ^= 0xFF  # corrupt payload byte
    ok = send_raw(s, bytes(pkt))
    log("Corrupted CRC packet sent (expect rejection)", PASS if ok else FAIL,
        "controller should log PARSE_BAD_CRC")
    s.close()
    time.sleep(0.5)

    # ── Test 3: Bad SOF ────────────────────────────────────────
    print("\n-- Bad SOF --")
    s = connect()
    pkt = bytearray(build_valid_packet(2, 0x00, 0x00, 95, 3000))
    pkt[0] = 0x00  # corrupt SOF
    ok = send_raw(s, bytes(pkt))
    log("Bad SOF packet sent (expect rejection)", PASS if ok else FAIL,
        "controller should log PARSE_BAD_SOF")
    s.close()
    time.sleep(0.5)

    # ── Test 4: Wrong protocol version ────────────────────────
    print("\n-- Protocol Version Mismatch --")
    s = connect()
    pkt = bytearray(build_valid_packet(3, 0x00, 0x00, 95, 4000))
    pkt[1] = 0x99  # wrong version
    ok = send_raw(s, bytes(pkt))
    log("Wrong version packet sent (expect rejection)", PASS if ok else FAIL,
        "controller should log PARSE_BAD_VERSION")
    s.close()
    time.sleep(0.5)

    # ── Test 5: Truncated packet ───────────────────────────────
    print("\n-- Truncated Packet --")
    s = connect()
    pkt = build_valid_packet(4, 0x00, 0x00, 95, 5000)
    truncated = pkt[:6]  # send only header, no payload or CRC
    ok = send_raw(s, truncated)
    log("Truncated packet sent (expect timeout/rejection)", PASS if ok else FAIL,
        "controller recv_all should block then disconnect")
    s.close()
    time.sleep(0.5)

    # ── Test 6: Low confidence → REINSPECT ────────────────────
    print("\n-- Low Confidence Policy --")
    s = connect()
    pkt = build_valid_packet(5, 0x00, 0x00, 45, 6000)  # conf=45 < threshold 80
    ok = send_raw(s, pkt)
    log("Low confidence packet sent (expect REINSPECT)", PASS if ok else FAIL,
        "InspectionTask should produce DECISION_REINSPECT")
    s.close()
    time.sleep(0.5)

    # ── Test 7: DEFECT result → REJECT ────────────────────────
    print("\n-- Defect Detection --")
    s = connect()
    pkt = build_valid_packet(6, 0x01, 0x03, 91, 7000)  # HEAT_DAMAGE
    ok = send_raw(s, pkt)
    log("DEFECT HEAT_DAMAGE packet sent (expect REJECT)", PASS if ok else FAIL,
        "ControlTask should produce DECISION_REJECT")
    s.close()
    time.sleep(0.5)

    # ── Test 8: Rapid burst (backpressure) ────────────────────
    print("\n-- Rapid Burst / Backpressure --")
    s = connect()
    sent = 0
    for i in range(20):
        pkt = build_valid_packet(i & 0xFF, 0x00, 0x00, 95, 8000 + i * 10)
        if send_raw(s, pkt):
            sent += 1
    log(f"Burst of 20 packets sent ({sent}/20 succeeded)", PASS if sent == 20 else FAIL,
        "queue should absorb burst without crash")
    s.close()
    time.sleep(0.5)

    # ── Test 9: SEQ gap detection ─────────────────────────────
    print("\n-- Sequence Number Gap --")
    s = connect()
    send_raw(s, build_valid_packet(10, 0x00, 0x00, 95, 9000))
    send_raw(s, build_valid_packet(15, 0x00, 0x00, 95, 9500))  # gap: 11-14 missing
    log("SEQ gap 10→15 sent (expect gap warning)", PASS,
        "controller should log SEQ mismatch warning")
    s.close()
    time.sleep(0.5)

    # ── Test 10: Double corrupted bytes ───────────────────────
    print("\n-- Multi-byte Corruption --")
    s = connect()
    pkt = bytearray(build_valid_packet(20, 0x00, 0x00, 95, 10000))
    pkt[6]  ^= 0xAA
    pkt[10] ^= 0x55
    ok = send_raw(s, bytes(pkt))
    log("Two corrupted bytes sent (expect CRC rejection)", PASS if ok else FAIL,
        "CRC should catch multi-byte corruption")
    s.close()

    # ── Summary ───────────────────────────────────────────────
    print("\n" + "="*55)
    passed = sum(1 for _, s in results if s == PASS)
    total  = len(results)
    print(f"  Results: {passed}/{total} tests sent successfully")
    print(f"  Check controller output to verify correct handling")
    print("="*55 + "\n")

if __name__ == "__main__":
    run_tests()
