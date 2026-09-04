add_library(freertos_kernel STATIC
    FreeRTOS-Kernel/tasks.c
    FreeRTOS-Kernel/queue.c
    FreeRTOS-Kernel/list.c
    FreeRTOS-Kernel/timers.c
    FreeRTOS-Kernel/event_groups.c
    FreeRTOS-Kernel/stream_buffer.c
    FreeRTOS-Kernel/portable/MSVC-MingW/port.c
    FreeRTOS-Kernel/portable/MemMang/heap_4.c
)

target_include_directories(freertos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    FreeRTOS-Kernel/include
    FreeRTOS-Kernel/portable/MSVC-MingW
)

target_compile_options(freertos_kernel PRIVATE
    -w
)
