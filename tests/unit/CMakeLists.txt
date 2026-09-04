find_package(GTest REQUIRED)

add_executable(unit_tests
    test_crc.cpp
    test_ring_buffer.cpp
    test_protocol.cpp
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    protocol
    hal
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
