#include "controller.h"
#include "firmware/hal/crc.h"
#include <stdio.h>
#include <string.h>

QueueHandle_t     xInspectionQueue = NULL;
QueueHandle_t     xDecisionQueue   = NULL;
QueueHandle_t     xLogQueue        = NULL;
SemaphoreHandle_t xStateMutex      = NULL;

volatile SystemState gSystemState = STATE_BOOT;

static volatile TickType_t xLastKick[WD_TASK_COUNT];

void LOG_Send(LogLevel level, const char *msg)
{
    if (xLogQueue == NULL || msg == NULL) return;
    LogMessage log;
    log.level        = level;
    log.timestamp_ms = (u32)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    strncpy(log.message, msg, sizeof(log.message) - 1U);
    log.message[sizeof(log.message) - 1U] = '\0';
    xQueueSend(xLogQueue, &log, 0);
}

void WATCHDOG_Kick(WatchdogTaskID id)
{
    if (id < WD_TASK_COUNT) {
        xLastKick[id] = xTaskGetTickCount();
    }
}

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
    printf("[ASSERT] %s:%lu\n", pcFile, ulLine);
    taskDISABLE_INTERRUPTS();
    for (;;) {}
}

void vCommunicationTask(void *pvParameters)
{
    (void)pvParameters;
    LOG_Send(LOG_INFO, "CommunicationTask started");
    u8 seq = 0;
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        WATCHDOG_Kick(WD_TASK_COMMUNICATION);
        InspectionMessage msg;
        msg.seq          = seq++;
        msg.timestamp_ms = (u32)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        switch (seq % 3) {
            case 0:
                msg.result       = INSPECT_GOOD;
                msg.confidence   = 95U;
                msg.defect_class = 0U;
                break;
            case 1:
                msg.result       = INSPECT_DEFECT;
                msg.confidence   = 88U;
                msg.defect_class = 1U;
                break;
            default:
                msg.result       = INSPECT_LOW_CONF;
                msg.confidence   = 55U;
                msg.defect_class = 0U;
                break;
        }
        if (xQueueSend(xInspectionQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
            LOG_Send(LOG_WARNING, "InspectionQueue full — message dropped");
        }
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(500));
    }
}

void vInspectionTask(void *pvParameters)
{
    (void)pvParameters;
    LOG_Send(LOG_INFO, "InspectionTask started");
    InspectionMessage msg;
    for (;;) {
        WATCHDOG_Kick(WD_TASK_INSPECTION);
        if (xQueueReceive(xInspectionQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
            MachineDecision decision;
            char log_buf[64];
            if (msg.result == INSPECT_LOW_CONF ||
                msg.confidence < CONFIDENCE_THRESHOLD) {
                decision = DECISION_REINSPECT;
                snprintf(log_buf, sizeof(log_buf),
                         "SEQ=%u LOW_CONF=%u -> REINSPECT", msg.seq, msg.confidence);
                LOG_Send(LOG_WARNING, log_buf);
            } else if (msg.result == INSPECT_GOOD) {
                decision = DECISION_ACCEPT;
                snprintf(log_buf, sizeof(log_buf),
                         "SEQ=%u GOOD conf=%u -> ACCEPT", msg.seq, msg.confidence);
                LOG_Send(LOG_INFO, log_buf);
            } else if (msg.result == INSPECT_DEFECT) {
                decision = DECISION_REJECT;
                snprintf(log_buf, sizeof(log_buf),
                         "SEQ=%u DEFECT class=%u conf=%u -> REJECT",
                         msg.seq, msg.defect_class, msg.confidence);
                LOG_Send(LOG_INFO, log_buf);
            } else {
                decision = DECISION_FAULT;
                LOG_Send(LOG_ERROR, "Invalid inspection result -> FAULT");
            }
            if (xQueueSend(xDecisionQueue, &decision, pdMS_TO_TICKS(100)) != pdTRUE) {
                LOG_Send(LOG_WARNING, "DecisionQueue full");
            }
        }
    }
}

void vControlTask(void *pvParameters)
{
    (void)pvParameters;
    LOG_Send(LOG_INFO, "ControlTask started");
    MachineDecision decision;
    u32 inspection_count = 0;
    for (;;) {
        WATCHDOG_Kick(WD_TASK_CONTROL);
        if (xQueueReceive(xDecisionQueue, &decision, pdMS_TO_TICKS(1000)) == pdTRUE) {
            inspection_count++;
            char buf[64];
            if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                switch (decision) {
                    case DECISION_ACCEPT:
                        gSystemState = STATE_COMPLETE;
                        snprintf(buf, sizeof(buf),
                                 "#%lu ACCEPT", (unsigned long)inspection_count);
                        LOG_Send(LOG_INFO, buf);
                        break;
                    case DECISION_REJECT:
                        gSystemState = STATE_COMPLETE;
                        snprintf(buf, sizeof(buf),
                                 "#%lu REJECT", (unsigned long)inspection_count);
                        LOG_Send(LOG_INFO, buf);
                        break;
                    case DECISION_REINSPECT:
                        gSystemState = STATE_INSPECTING;
                        snprintf(buf, sizeof(buf),
                                 "#%lu REINSPECT", (unsigned long)inspection_count);
                        LOG_Send(LOG_WARNING, buf);
                        break;
                    case DECISION_FAULT:
                        gSystemState = STATE_FAULT;
                        LOG_Send(LOG_ERROR, "FAULT - entering safe state");
                        break;
                }
                xSemaphoreGive(xStateMutex);
            }
        }
    }
}

void vWatchdogTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t now = xTaskGetTickCount();
    for (int i = 0; i < WD_TASK_COUNT; i++) {
        xLastKick[i] = now;
    }
    const char *task_names[WD_TASK_COUNT] = {
        "CommunicationTask", "InspectionTask",
        "ControlTask",       "DiagnosticsTask"
    };
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(200));
        now = xTaskGetTickCount();
        TickType_t timeout = pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS);
        for (int i = 0; i < WD_TASK_COUNT; i++) {
            if ((now - xLastKick[i]) > timeout) {
                char buf[64];
                snprintf(buf, sizeof(buf), "WATCHDOG: %s not responding!",
                         task_names[i]);
                LOG_Send(LOG_ERROR, buf);
                if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    gSystemState = STATE_FAULT;
                    xSemaphoreGive(xStateMutex);
                }
            }
        }
    }
}

void vDiagnosticsTask(void *pvParameters)
{
    (void)pvParameters;
    LOG_Send(LOG_INFO, "DiagnosticsTask started");

    static BaseType_t report_done = pdFALSE;

    for (;;) {
        WATCHDOG_Kick(WD_TASK_DIAGNOSTICS);
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Print memory report once after 10 seconds */
        if (report_done == pdFALSE &&
            xTaskGetTickCount() > pdMS_TO_TICKS(10000)) {

            printf("\n========================================\n");
            printf("  MEMORY MODEL REPORT\n");
            printf("========================================\n");
            printf("  Heap total : %u bytes\n",
                   (unsigned)configTOTAL_HEAP_SIZE);
            printf("  Heap free  : %u bytes\n",
                   (unsigned)xPortGetFreeHeapSize());
            printf("  Heap used  : %u bytes\n",
                   (unsigned)(configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize()));
            printf("----------------------------------------\n");
            printf("  %-16s  %s\n", "Task", "Stack HWM (words free)");
            printf("----------------------------------------\n");

            TaskStatus_t tasks[12];
            UBaseType_t count = uxTaskGetSystemState(tasks, 12, NULL);
            for (UBaseType_t i = 0; i < count; i++) {
                printf("  %-16s  %u\n",
                       tasks[i].pcTaskName,
                       (unsigned)tasks[i].usStackHighWaterMark);
            }
            printf("========================================\n\n");
            fflush(stdout);
            report_done = pdTRUE;
        }

        char buf[64];
        SystemState state;
        if (xSemaphoreTake(xStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            state = gSystemState;
            xSemaphoreGive(xStateMutex);
        } else {
            state = STATE_FAULT;
        }
        const char *state_str[] = {
            "BOOT","SELFTEST","IDLE","INSPECTING","COMPLETE","FAULT"
        };
        snprintf(buf, sizeof(buf), "DIAG: state=%s insp_q=%u dec_q=%u",
                 state_str[state],
                 (unsigned)uxQueueMessagesWaiting(xInspectionQueue),
                 (unsigned)uxQueueMessagesWaiting(xDecisionQueue));
        LOG_Send(LOG_DEBUG, buf);
    }
}

void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    const char *level_str[] = { "DEBUG", "INFO ", "WARN ", "ERROR" };
    LogMessage log;
    for (;;) {
        if (xQueueReceive(xLogQueue, &log, portMAX_DELAY) == pdTRUE) {
            printf("[%6lu] [%s] %s\n",
                   (unsigned long)log.timestamp_ms,
                   level_str[log.level],
                   log.message);
            fflush(stdout);
        }
    }
}
