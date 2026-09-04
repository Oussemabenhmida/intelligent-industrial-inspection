cmake_minimum_required(VERSION 3.20)

project(IntelligentIndustrialInspection
    VERSION 0.1.0
    DESCRIPTION "Intelligent Industrial Edge-AI Inspection and Real-Time Control System"
    LANGUAGES C CXX
)

# C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")

# Subdirectories (uncomment as we build each one)
add_subdirectory(firmware)
# add_subdirectory(tests)
add_subdirectory(protocol)
