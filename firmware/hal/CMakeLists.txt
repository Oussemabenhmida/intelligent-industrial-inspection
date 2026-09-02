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
