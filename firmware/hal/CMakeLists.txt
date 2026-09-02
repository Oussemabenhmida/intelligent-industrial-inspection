add_library(hal
    hal_gpio.c
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
