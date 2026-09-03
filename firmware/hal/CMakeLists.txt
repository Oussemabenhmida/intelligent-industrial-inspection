add_library(hal
    hal_gpio.c
    hal_uart.c
    ring_buffer.c
)

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(hal PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(gpio_smoke_test
    gpio_smoke_test.c
)
target_link_libraries(gpio_smoke_test PRIVATE hal)

add_executable(uart_smoke_test
    uart_smoke_test.c
)
target_link_libraries(uart_smoke_test PRIVATE hal)

add_executable(rb_smoke_test
    rb_smoke_test.c
)
target_link_libraries(rb_smoke_test PRIVATE hal)
