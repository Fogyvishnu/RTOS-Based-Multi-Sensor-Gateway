cmake_minimum_required(VERSION 3.10)
project(RTOS_Multi_Sensor_Gateway C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to build host tests
option(BUILD_TESTS "Build host unit test suite" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
