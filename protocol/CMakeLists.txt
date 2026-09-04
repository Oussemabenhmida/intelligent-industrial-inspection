add_library(protocol
    packet.c
)

target_include_directories(protocol PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(protocol PUBLIC hal)

target_compile_options(protocol PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(proto_smoke_test
    proto_smoke_test.c
)
target_link_libraries(proto_smoke_test PRIVATE protocol)
