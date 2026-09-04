#include "packet.h"
#include <string.h>

/* ─────────────────────────────────────────────
   PROTO_Serialize

   Writes a Packet into a flat byte buffer
   ready for transmission over UART.

   Layout written:
   [SOF][VER][SEQ][ID][LEN_LO][LEN_HI][PAYLOAD...][CRC_LO][CRC_HI]

   CRC is computed over header + payload.
   Returns total bytes written, 0 on error.
───────────────────────────────────────────── */

u16 PROTO_Serialize(const Packet *pkt, u8 *buf, u16 buf_size)
{
    if (pkt == NULL || buf == NULL) return 0U;

    u16 total = (u16)(PACKET_HEADER_SIZE + pkt->header.length + PACKET_CRC_SIZE);
    if (total > buf_size || pkt->header.length > PROTO_MAX_PAYLOAD) return 0U;

    u16 idx = 0U;

    /* Header */
    buf[idx++] = pkt->header.sof;
    buf[idx++] = pkt->header.version;
    buf[idx++] = pkt->header.seq;
    buf[idx++] = pkt->header.msg_id;
    buf[idx++] = (u8)(pkt->header.length & 0xFFU);        /* length low byte  */
    buf[idx++] = (u8)((pkt->header.length >> 8U) & 0xFFU); /* length high byte */

    /* Payload */
    memcpy(&buf[idx], pkt->payload, pkt->header.length);
    idx += pkt->header.length;

    /* CRC over header + payload */
    u16 crc = CRC16_Compute(buf, idx);
    buf[idx++] = (u8)(crc & 0xFFU);
    buf[idx++] = (u8)((crc >> 8U) & 0xFFU);

    return idx;
}

/* ─────────────────────────────────────────────
   PROTO_Deserialize

   Parses a flat byte buffer into a Packet.
   Validates SOF, version, length, and CRC.
   Returns a ParseResult code.
───────────────────────────────────────────── */

ParseResult PROTO_Deserialize(const u8 *buf, u16 len, Packet *pkt)
{
    if (buf == NULL || pkt == NULL) return PARSE_NULL;
    if (len < (u16)(PACKET_HEADER_SIZE + PACKET_CRC_SIZE)) return PARSE_BAD_LENGTH;

    /* SOF check */
    if (buf[0] != PROTO_SOF) return PARSE_BAD_SOF;

    /* Version check */
    if (buf[1] != PROTO_VERSION) return PARSE_BAD_VERSION;

    /* Parse header */
    pkt->header.sof     = buf[0];
    pkt->header.version = buf[1];
    pkt->header.seq     = buf[2];
    pkt->header.msg_id  = buf[3];
    pkt->header.length  = (u16)(buf[4] | ((u16)buf[5] << 8U));

    /* Length sanity check */
    if (pkt->header.length > PROTO_MAX_PAYLOAD) return PARSE_BAD_LENGTH;

    u16 expected_total = (u16)(PACKET_HEADER_SIZE + pkt->header.length + PACKET_CRC_SIZE);
    if (len < expected_total) return PARSE_BAD_LENGTH;

    /* CRC check — covers header + payload */
    u16 data_len = (u16)(PACKET_HEADER_SIZE + pkt->header.length);
    u16 computed = CRC16_Compute(buf, data_len);
    u16 received = (u16)(buf[data_len] | ((u16)buf[data_len + 1U] << 8U));
    if (computed != received) return PARSE_BAD_CRC;

    /* Copy payload */
    memcpy(pkt->payload, &buf[PACKET_HEADER_SIZE], pkt->header.length);
    pkt->crc = received;

    return PARSE_OK;
}

/* ─────────────────────────────────────────────
   PROTO_BuildInspection

   Fills a Packet with an inspection result.
───────────────────────────────────────────── */

void PROTO_BuildInspection(Packet *pkt, u8 seq,
                           InspectResult result,
                           u8 defect_class,
                           u8 confidence,
                           u32 timestamp_ms)
{
    if (pkt == NULL) return;

    pkt->header.sof     = PROTO_SOF;
    pkt->header.version = PROTO_VERSION;
    pkt->header.seq     = seq;
    pkt->header.msg_id  = MSG_INSPECTION;
    pkt->header.length  = sizeof(InspectionPayload);

    InspectionPayload *p = (InspectionPayload *)pkt->payload;
    p->result       = (u8)result;
    p->defect_class = defect_class;
    p->confidence   = confidence;
    p->timestamp_ms = timestamp_ms;
}

/* ─────────────────────────────────────────────
   PROTO_BuildHeartbeat

   Builds a minimal heartbeat packet.
───────────────────────────────────────────── */

void PROTO_BuildHeartbeat(Packet *pkt, u8 seq)
{
    if (pkt == NULL) return;

    pkt->header.sof     = PROTO_SOF;
    pkt->header.version = PROTO_VERSION;
    pkt->header.seq     = seq;
    pkt->header.msg_id  = MSG_HEARTBEAT;
    pkt->header.length  = 0U;
}

/* ─────────────────────────────────────────────
   PROTO_IsValidSeq

   Checks sequence number is as expected.
   Wraps correctly at 255 -> 0.
───────────────────────────────────────────── */

bool PROTO_IsValidSeq(u8 expected, u8 received)
{
    return expected == received;
}
