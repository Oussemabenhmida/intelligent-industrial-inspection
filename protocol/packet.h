#ifndef PACKET_H
#define PACKET_H

#include "firmware/hal/hal_types.h"
#include "firmware/hal/crc.h"

/* ─────────────────────────────────────────────
   Protocol constants
───────────────────────────────────────────── */

#define PROTO_SOF           0xAAU   /* Start of frame marker        */
#define PROTO_VERSION       0x01U   /* Protocol version             */
#define PROTO_MAX_PAYLOAD   64U     /* Maximum payload bytes        */

/* ─────────────────────────────────────────────
   Message IDs
───────────────────────────────────────────── */

typedef enum {
    MSG_HEARTBEAT       = 0x01,
    MSG_INSPECTION      = 0x02,
    MSG_ACK             = 0x03,
    MSG_NACK            = 0x04,
    MSG_FAULT           = 0x05,
    MSG_HELLO           = 0x06,
    MSG_READY           = 0x07,
    MSG_VERSION         = 0x08
} MsgID;

/* ─────────────────────────────────────────────
   Inspection result values
───────────────────────────────────────────── */

typedef enum {
    INSPECT_GOOD        = 0x00,
    INSPECT_DEFECT      = 0x01,
    INSPECT_LOW_CONF    = 0x02,
    INSPECT_INVALID     = 0xFF
} InspectResult;

/* ─────────────────────────────────────────────
   Packet header — fixed 6 bytes
   SOF | VER | SEQ | ID | LENGTH (2 bytes LE)
───────────────────────────────────────────── */

typedef struct {
    u8  sof;        /* Always PROTO_SOF             */
    u8  version;    /* Protocol version             */
    u8  seq;        /* Sequence number 0-255        */
    u8  msg_id;     /* Message type                 */
    u16 length;     /* Payload length in bytes      */
} __attribute__((packed)) PacketHeader;

/* ─────────────────────────────────────────────
   Full packet — header + payload + CRC
───────────────────────────────────────────── */

#define PACKET_HEADER_SIZE  sizeof(PacketHeader)    /* 6 bytes */
#define PACKET_CRC_SIZE     2U                       /* CRC-16  */
#define PACKET_MAX_SIZE     (PACKET_HEADER_SIZE + PROTO_MAX_PAYLOAD + PACKET_CRC_SIZE)

typedef struct {
    PacketHeader header;
    u8           payload[PROTO_MAX_PAYLOAD];
    u16          crc;
} Packet;

/* ─────────────────────────────────────────────
   Inspection payload — carried inside MSG_INSPECTION
───────────────────────────────────────────── */

typedef struct {
    u8  result;         /* InspectResult            */
    u8  defect_class;   /* Defect class ID          */
    u8  confidence;     /* 0-100 scaled             */
    u32 timestamp_ms;   /* Milliseconds since boot  */
} __attribute__((packed)) InspectionPayload;

/* ─────────────────────────────────────────────
   Parse result codes
───────────────────────────────────────────── */

typedef enum {
    PARSE_OK            = 0,
    PARSE_BAD_SOF       = 1,
    PARSE_BAD_VERSION   = 2,
    PARSE_BAD_LENGTH    = 3,
    PARSE_BAD_CRC       = 4,
    PARSE_NULL          = 5
} ParseResult;

/* ─────────────────────────────────────────────
   API
───────────────────────────────────────────── */

u16         PROTO_Serialize(const Packet *pkt, u8 *buf, u16 buf_size);
ParseResult PROTO_Deserialize(const u8 *buf, u16 len, Packet *pkt);
void        PROTO_BuildInspection(Packet *pkt, u8 seq,
                                  InspectResult result,
                                  u8 defect_class,
                                  u8 confidence,
                                  u32 timestamp_ms);
void        PROTO_BuildHeartbeat(Packet *pkt, u8 seq);
bool        PROTO_IsValidSeq(u8 expected, u8 received);

#endif /* PACKET_H */
