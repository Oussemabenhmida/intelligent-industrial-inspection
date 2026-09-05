#include "tcp_listener.h"
#include "protocol/packet.h"
#include "firmware/hal/crc.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#define WOULD_BLOCK (WSAGetLastError() == WSAEWOULDBLOCK)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#define closesocket close
#define WOULD_BLOCK (errno == EAGAIN || errno == EWOULDBLOCK)
#endif

/* Receive exactly 'len' bytes — yields to scheduler between attempts */
static int recv_all(int fd, u8 *buf, int len)
{
    int total = 0;
    while (total < len) {
        int n = recv(fd, (char *)buf + total, len - total, 0);
        if (n > 0) {
            total += n;
        } else if (n == 0) {
            return 0;  /* connection closed */
        } else {
            if (WOULD_BLOCK) {
                vTaskDelay(pdMS_TO_TICKS(5));
                continue;
            }
            return -1;  /* real error */
        }
    }
    return total;
}

void vTCPListenerTask(void *pvParameters)
{
    (void)pvParameters;

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    printf("[TCP] Starting listener on port 5000\n"); fflush(stdout);

    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("[TCP] socket failed\n"); fflush(stdout);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&opt, sizeof(opt));

    /* Non-blocking server socket */
#ifdef _WIN32
    u_long nb = 1;
    ioctlsocket(server_fd, FIONBIO, &nb);
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(TCP_LISTENER_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("[TCP] bind failed\n"); fflush(stdout);
        closesocket(server_fd);
        vTaskDelete(NULL);
        return;
    }

    listen(server_fd, 1);
    WATCHDOG_Kick(WD_TASK_COMMUNICATION);
    printf("[TCP] Waiting for connection...\n"); fflush(stdout);

    for (;;)
    {
        /* Non-blocking accept — yield if no connection yet */
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = (int)accept(server_fd,
                                    (struct sockaddr *)&client_addr,
                                    &client_len);
        if (client_fd < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            WATCHDOG_Kick(WD_TASK_COMMUNICATION);
            continue;
        }

        /* Set client socket non-blocking */
#ifdef _WIN32
        u_long nb2 = 1;
        ioctlsocket(client_fd, FIONBIO, &nb2);
#endif

        printf("[TCP] Jetson connected!\n"); fflush(stdout);

        u8 buf[PACKET_MAX_SIZE];
        Packet pkt;

        for (;;)
        {
            WATCHDOG_Kick(WD_TASK_COMMUNICATION);
            /* Read header */
            int n = recv_all(client_fd, buf, (int)PACKET_HEADER_SIZE);
            if (n <= 0) {
                printf("[TCP] Connection closed\n"); fflush(stdout);
                break;
            }

            u16 payload_len = (u16)(buf[4] | ((u16)buf[5] << 8));
            if (payload_len > PROTO_MAX_PAYLOAD) {
                printf("[TCP] Oversized payload %u\n", payload_len);
                fflush(stdout);
                break;
            }

            /* Read payload + CRC */
            u16 remaining = payload_len + (u16)PACKET_CRC_SIZE;
            n = recv_all(client_fd, &buf[PACKET_HEADER_SIZE], (int)remaining);
            if (n <= 0) break;

            u16 total = (u16)(PACKET_HEADER_SIZE + remaining);
            ParseResult r = PROTO_Deserialize(buf, total, &pkt);

            printf("[TCP] SEQ=%u parse=%d result=%u conf=%u\n",
                   pkt.header.seq, (int)r,
                   pkt.payload[0], pkt.payload[2]);
            fflush(stdout);

            if (r != PARSE_OK) continue;

            if (pkt.header.msg_id == MSG_INSPECTION) {
                InspectionPayload *p = (InspectionPayload *)pkt.payload;
                InspectionMessage msg;
                msg.seq          = pkt.header.seq;
                msg.result       = p->result;
                msg.confidence   = p->confidence;
                msg.defect_class = p->defect_class;
                msg.timestamp_ms = p->timestamp_ms;

                BaseType_t sent = xQueueSend(xInspectionQueue, &msg,
                                             pdMS_TO_TICKS(100));
                printf("[TCP] Queue: %s\n", sent == pdTRUE ? "OK" : "FAILED");
                fflush(stdout);
            }
        }

        closesocket(client_fd);
        printf("[TCP] Disconnected\n"); fflush(stdout);
    }
}
/* watchdog kick added via patch — see controller.h */
