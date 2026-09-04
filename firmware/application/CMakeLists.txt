add_executable(controller
    main_controller.c
    controller.c
)

target_include_directories(controller PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/firmware/rtos
    ${CMAKE_SOURCE_DIR}/firmware/rtos/FreeRTOS-Kernel/include
    ${CMAKE_SOURCE_DIR}/firmware/rtos/FreeRTOS-Kernel/portable/MSVC-MingW
)

target_link_libraries(controller PRIVATE
    freertos_kernel
    protocol
    hal
    winmm
    ws2_32
)

target_compile_options(controller PRIVATE
    -Wall
    -Wextra
)
