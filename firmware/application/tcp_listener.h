#ifndef TCP_LISTENER_H
#define TCP_LISTENER_H

#include "controller.h"

/* ─────────────────────────────────────────────
   TCP listener task
   Accepts connections from the Jetson sender.
   Feeds received packets into xInspectionQueue
   exactly like the simulated CommunicationTask.
───────────────────────────────────────────── */

#define TCP_LISTENER_PORT   5000
#define TCP_STACK_SIZE      1024

void vTCPListenerTask(void *pvParameters);

#endif /* TCP_LISTENER_H */
