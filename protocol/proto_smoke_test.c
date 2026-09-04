#include <stdio.h>
#include <string.h>
#include "protocol/packet.h"

static void print_packet(const char *label, const Packet *pkt)
{
    printf("\n[%s]\n", label);
    printf("  SOF     : 0x%02X\n", pkt->header.sof);
    printf("  VER     : 0x%02X\n", pkt->header.version);
    printf("  SEQ     : %u\n",     pkt->header.seq);
    printf("  MSG_ID  : 0x%02X\n", pkt->header.msg_id);
    printf("  LENGTH  : %u\n",     pkt->header.length);
    printf("  CRC     : 0x%04X\n", pkt->crc);
}

static void print_buf(const char *label, const u8 *buf, u16 len)
{
    printf("[%s] %u bytes: ", label, len);
    for (u16 i = 0; i < len; i++) printf("%02X ", buf[i]);
    printf("\n");
}

int main(void)
{
    printf("=== Protocol Smoke Test ===\n");

    CRC16_Init();

    u8 buf[PACKET_MAX_SIZE];
    Packet tx, rx;
    u16 written;
    ParseResult result;

    /* ── Test 1: build and serialize inspection packet ── */
    printf("\n--- Build inspection packet ---\n");
    memset(&tx, 0, sizeof(tx));
    PROTO_BuildInspection(&tx, 1U, INSPECT_GOOD, 0x00, 97U, 12345U);
    print_packet("TX inspection", &tx);

    written = PROTO_Serialize(&tx, buf, sizeof(buf));
    printf("Serialized: %u bytes\n", written);
    print_buf("raw", buf, written);

    /* ── Test 2: deserialize and verify ── */
    printf("\n--- Deserialize ---\n");
    memset(&rx, 0, sizeof(rx));
    result = PROTO_Deserialize(buf, written, &rx);
    printf("ParseResult: %s\n", result == PARSE_OK ? "PARSE_OK" : "FAIL");
    print_packet("RX inspection", &rx);

    InspectionPayload *p = (InspectionPayload *)rx.payload;
    printf("  result      : %u (expect %u)\n", p->result,       INSPECT_GOOD);
    printf("  defect_class: %u (expect 0)\n",  p->defect_class);
    printf("  confidence  : %u (expect 97)\n", p->confidence);
    printf("  timestamp   : %u (expect 12345)\n", p->timestamp_ms);

    /* ── Test 3: CRC corruption detected ── */
    printf("\n--- CRC corruption detection ---\n");
    u8 corrupt[PACKET_MAX_SIZE];
    memcpy(corrupt, buf, written);
    corrupt[7] ^= 0xFF;   /* corrupt payload byte */
    memset(&rx, 0, sizeof(rx));
    result = PROTO_Deserialize(corrupt, written, &rx);
    printf("Corrupted packet result: %s\n",
           result == PARSE_BAD_CRC ? "PARSE_BAD_CRC (correct)" : "WRONG");

    /* ── Test 4: bad SOF detected ── */
    printf("\n--- Bad SOF detection ---\n");
    u8 bad_sof[PACKET_MAX_SIZE];
    memcpy(bad_sof, buf, written);
    bad_sof[0] = 0x00;
    result = PROTO_Deserialize(bad_sof, written, &rx);
    printf("Bad SOF result: %s\n",
           result == PARSE_BAD_SOF ? "PARSE_BAD_SOF (correct)" : "WRONG");

    /* ── Test 5: bad version detected ── */
    printf("\n--- Bad version detection ---\n");
    u8 bad_ver[PACKET_MAX_SIZE];
    memcpy(bad_ver, buf, written);
    bad_ver[1] = 0x99;
    result = PROTO_Deserialize(bad_ver, written, &rx);
    printf("Bad version result: %s\n",
           result == PARSE_BAD_VERSION ? "PARSE_BAD_VERSION (correct)" : "WRONG");

    /* ── Test 6: heartbeat packet ── */
    printf("\n--- Heartbeat packet ---\n");
    memset(&tx, 0, sizeof(tx));
    PROTO_BuildHeartbeat(&tx, 2U);
    written = PROTO_Serialize(&tx, buf, sizeof(buf));
    printf("Heartbeat serialized: %u bytes\n", written);
    print_buf("raw", buf, written);
    memset(&rx, 0, sizeof(rx));
    result = PROTO_Deserialize(buf, written, &rx);
    printf("Heartbeat parse: %s\n", result == PARSE_OK ? "PARSE_OK" : "FAIL");

    /* ── Test 7: sequence number check ── */
    printf("\n--- Sequence number ---\n");
    printf("SEQ valid (5==5): %s\n",   PROTO_IsValidSeq(5, 5) ? "true" : "false");
    printf("SEQ invalid (5==6): %s\n", PROTO_IsValidSeq(5, 6) ? "true" : "false");

    /* ── Test 8: NULL guards ── */
    printf("\n--- NULL guards ---\n");
    written = PROTO_Serialize(NULL, buf, sizeof(buf));
    printf("Serialize(NULL pkt): %u (expect 0)\n", written);
    result = PROTO_Deserialize(NULL, 10, &rx);
    printf("Deserialize(NULL buf): %s\n",
           result == PARSE_NULL ? "PARSE_NULL (correct)" : "WRONG");

    printf("\n=== Protocol smoke test complete ===\n");
    return 0;
}
