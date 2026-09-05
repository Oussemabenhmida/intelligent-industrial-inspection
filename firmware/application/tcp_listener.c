#include "tcp_listener.h"
#include "protocol/packet.h"
#include "firmware/hal/crc.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define closesocket close
#endif

/* ─────────────────────────────────────────────
   vTCPListenerTask

   Listens on TCP_LISTENER_PORT.
   Accepts one connection at a time from Jetson.
   Reads binary protocol packets.
   Deserializes and pushes to xInspectionQueue.
───────────────────────────────────────────── */

void vTCPListenerTask(void *pvParameters)
{
    (void)pvParameters;

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    LOG_Send(LOG_INFO, "TCPListener: starting on port 5000");

    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        LOG_Send(LOG_ERROR, "TCPListener: socket failed");
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(TCP_LISTENER_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_Send(LOG_ERROR, "TCPListener: bind failed");
        closesocket(server_fd);
        vTaskDelete(NULL);
        return;
    }

    listen(server_fd, 1);
    LOG_Send(LOG_INFO, "TCPListener: waiting for Jetson connection");

    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = (int)accept(server_fd,
                                    (struct sockaddr *)&client_addr,
                                    &client_len);
        if (client_fd < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        LOG_Send(LOG_INFO, "TCPListener: Jetson connected");

        u8 buf[PACKET_MAX_SIZE];
        Packet pkt;
        u8 seq_expected = 0;

        for (;;)
        {
            /* Read header first to know total length */
            int hdr_bytes = recv(client_fd, (char *)buf,
                                 (int)PACKET_HEADER_SIZE, MSG_WAITALL);
            if (hdr_bytes <= 0) break;

            u16 payload_len = (u16)(buf[4] | ((u16)buf[5] << 8));
            if (payload_len > PROTO_MAX_PAYLOAD) {
                LOG_Send(LOG_WARNING, "TCPListener: oversized payload");
                break;
            }

            /* Read payload + CRC */
            u16 remaining = payload_len + (u16)PACKET_CRC_SIZE;
            int body_bytes = recv(client_fd,
                                  (char *)&buf[PACKET_HEADER_SIZE],
                                  remaining, MSG_WAITALL);
            if (body_bytes <= 0) break;

            u16 total = (u16)(PACKET_HEADER_SIZE + remaining);
            ParseResult r = PROTO_Deserialize(buf, total, &pkt);

            if (r != PARSE_OK) {
                char log[48];
                snprintf(log, sizeof(log),
                         "TCPListener: parse error %d", (int)r);
                LOG_Send(LOG_WARNING, log);
                continue;
            }

            /* Sequence check */
            if (!PROTO_IsValidSeq(seq_expected, pkt.header.seq)) {
                char log[64];
                snprintf(log, sizeof(log),
                         "TCPListener: SEQ mismatch exp=%u got=%u",
                         seq_expected, pkt.header.seq);
                LOG_Send(LOG_WARNING, log);
            }
            seq_expected = (pkt.header.seq + 1U) & 0xFF;

            /* Push to inspection queue */
            if (pkt.header.msg_id == MSG_INSPECTION) {
                InspectionPayload *p = (InspectionPayload *)pkt.payload;
                InspectionMessage msg;
                msg.seq          = pkt.header.seq;
                msg.result       = p->result;
                msg.confidence   = p->confidence;
                msg.defect_class = p->defect_class;
                msg.timestamp_ms = p->timestamp_ms;

                if (xQueueSend(xInspectionQueue, &msg,
                               pdMS_TO_TICKS(100)) != pdTRUE) {
                    LOG_Send(LOG_WARNING, "TCPListener: queue full");
                }
            }
        }

        closesocket(client_fd);
        LOG_Send(LOG_WARNING, "TCPListener: Jetson disconnected");
    }
}
