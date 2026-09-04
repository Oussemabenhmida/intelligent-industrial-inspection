#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "firmware/hal/hal_types.h"
#include "protocol/packet.h"

/* ─────────────────────────────────────────────
   Task priorities
   Higher number = higher priority.
   Watchdog is highest — must always run.
───────────────────────────────────────────── */

#define PRIORITY_WATCHDOG       7
#define PRIORITY_COMMUNICATION  5
#define PRIORITY_INSPECTION     4
#define PRIORITY_CONTROL        4
#define PRIORITY_DIAGNOSTICS    3
#define PRIORITY_LOGGER         2

/* ─────────────────────────────────────────────
   Task stack sizes (in words, not bytes)
───────────────────────────────────────────── */

#define STACK_WATCHDOG          256
#define STACK_COMMUNICATION     512
#define STACK_INSPECTION        512
#define STACK_CONTROL           512
#define STACK_DIAGNOSTICS       256
#define STACK_LOGGER            256

/* ─────────────────────────────────────────────
   Queue depths
───────────────────────────────────────────── */

#define QUEUE_INSPECTION_DEPTH  8
#define QUEUE_DECISION_DEPTH    8
#define QUEUE_LOG_DEPTH         16

/* ─────────────────────────────────────────────
   Watchdog kick timeout (ms)
   Each task must kick within this window.
───────────────────────────────────────────── */

#define WATCHDOG_TIMEOUT_MS     3000U

/* ─────────────────────────────────────────────
   Machine decision
───────────────────────────────────────────── */

typedef enum {
    DECISION_ACCEPT     = 0,
    DECISION_REJECT     = 1,
    DECISION_REINSPECT  = 2,
    DECISION_FAULT      = 3
} MachineDecision;

/* ─────────────────────────────────────────────
   Inspection message — passed through queue
───────────────────────────────────────────── */

typedef struct {
    u8  result;
    u8  confidence;
    u8  defect_class;
    u32 timestamp_ms;
    u8  seq;
} InspectionMessage;

/* ─────────────────────────────────────────────
   System state machine
───────────────────────────────────────────── */

typedef enum {
    STATE_BOOT          = 0,
    STATE_SELFTEST      = 1,
    STATE_IDLE          = 2,
    STATE_INSPECTING    = 3,
    STATE_COMPLETE      = 4,
    STATE_FAULT         = 5
} SystemState;

/* ─────────────────────────────────────────────
   Log level
───────────────────────────────────────────── */

typedef enum {
    LOG_DEBUG   = 0,
    LOG_INFO    = 1,
    LOG_WARNING = 2,
    LOG_ERROR   = 3
} LogLevel;

/* ─────────────────────────────────────────────
   Log message — passed through log queue
───────────────────────────────────────────── */

typedef struct {
    LogLevel level;
    char     message[64];
    u32      timestamp_ms;
} LogMessage;

/* ─────────────────────────────────────────────
   Watchdog kick IDs — one per task
───────────────────────────────────────────── */

typedef enum {
    WD_TASK_COMMUNICATION   = 0,
    WD_TASK_INSPECTION      = 1,
    WD_TASK_CONTROL         = 2,
    WD_TASK_DIAGNOSTICS     = 3,
    WD_TASK_COUNT           = 4
} WatchdogTaskID;

/* ─────────────────────────────────────────────
   Shared queue handles — defined in controller.c
───────────────────────────────────────────── */

extern QueueHandle_t  xInspectionQueue;
extern QueueHandle_t  xDecisionQueue;
extern QueueHandle_t  xLogQueue;
extern SemaphoreHandle_t xStateMutex;

/* ─────────────────────────────────────────────
   Shared state — protected by xStateMutex
───────────────────────────────────────────── */

extern volatile SystemState gSystemState;

/* ─────────────────────────────────────────────
   Confidence threshold (0-100)
───────────────────────────────────────────── */

#define CONFIDENCE_THRESHOLD    80U

/* ─────────────────────────────────────────────
   Task functions
───────────────────────────────────────────── */

void vCommunicationTask(void *pvParameters);
void vInspectionTask(void *pvParameters);
void vControlTask(void *pvParameters);
void vWatchdogTask(void *pvParameters);
void vDiagnosticsTask(void *pvParameters);
void vLoggerTask(void *pvParameters);

/* ─────────────────────────────────────────────
   Watchdog API
───────────────────────────────────────────── */

void WATCHDOG_Kick(WatchdogTaskID id);

/* ─────────────────────────────────────────────
   Logging API
───────────────────────────────────────────── */

void LOG_Send(LogLevel level, const char *msg);

/* ─────────────────────────────────────────────
   Assert handler
───────────────────────────────────────────── */

void vAssertCalled(const char *pcFile, unsigned long ulLine);

#endif /* CONTROLLER_H */
