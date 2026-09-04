#include "controller.h"
#include "firmware/hal/crc.h"
#include <stdio.h>

/* ─────────────────────────────────────────────
   FreeRTOS hook functions — required by kernel
───────────────────────────────────────────── */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
    (void)xTask;
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    printf("[FATAL] Malloc failed — heap exhausted\n");
    for (;;) {}
}

/* ─────────────────────────────────────────────
   Main entry point
───────────────────────────────────────────── */

int main(void)
{
    printf("=== Intelligent Industrial Inspection Controller ===\n");
    printf("Initializing...\n\n");

    CRC16_Init();

    xInspectionQueue = xQueueCreate(QUEUE_INSPECTION_DEPTH, sizeof(InspectionMessage));
    xDecisionQueue   = xQueueCreate(QUEUE_DECISION_DEPTH,   sizeof(MachineDecision));
    xLogQueue        = xQueueCreate(QUEUE_LOG_DEPTH,        sizeof(LogMessage));

    configASSERT(xInspectionQueue != NULL);
    configASSERT(xDecisionQueue   != NULL);
    configASSERT(xLogQueue        != NULL);

    xStateMutex = xSemaphoreCreateMutex();
    configASSERT(xStateMutex != NULL);

    gSystemState = STATE_IDLE;

    xTaskCreate(vLoggerTask,        "Logger",   STACK_LOGGER,        NULL, PRIORITY_LOGGER,        NULL);
    xTaskCreate(vWatchdogTask,      "Watchdog", STACK_WATCHDOG,      NULL, PRIORITY_WATCHDOG,       NULL);
    xTaskCreate(vDiagnosticsTask,   "Diag",     STACK_DIAGNOSTICS,   NULL, PRIORITY_DIAGNOSTICS,    NULL);
    xTaskCreate(vCommunicationTask, "CommTask", STACK_COMMUNICATION, NULL, PRIORITY_COMMUNICATION,  NULL);
    xTaskCreate(vInspectionTask,    "InspTask", STACK_INSPECTION,    NULL, PRIORITY_INSPECTION,     NULL);
    xTaskCreate(vControlTask,       "CtrlTask", STACK_CONTROL,       NULL, PRIORITY_CONTROL,        NULL);

    printf("Tasks created. Starting scheduler...\n\n");

    vTaskStartScheduler();

    printf("ERROR: Scheduler returned\n");
    return 1;
}
