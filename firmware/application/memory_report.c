#include "memory_report.h"
#include "controller.h"
#include <stdio.h>

/* ─────────────────────────────────────────────
   MEMORY_PrintReport

   Prints FreeRTOS task stack usage.
   Call once after system has been running
   for at least a few seconds to get meaningful
   high-water marks.
───────────────────────────────────────────── */

void MEMORY_PrintReport(void)
{
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
    printf("  %-16s %8s %8s %8s\n",
           "Task", "Stack(W)", "HWM(W)", "Used(W)");
    printf("----------------------------------------\n");

    TaskStatus_t tasks[10];
    UBaseType_t count = uxTaskGetSystemState(tasks, 10, NULL);

    for (UBaseType_t i = 0; i < count; i++) {
        UBaseType_t hwm   = tasks[i].usStackHighWaterMark;
        UBaseType_t total = tasks[i].usStackHighWaterMark; /* approximation */
        printf("  %-16s %8u %8u\n",
               tasks[i].pcTaskName,
               (unsigned)hwm,
               (unsigned)hwm);
    }

    printf("========================================\n\n");
    fflush(stdout);
}
