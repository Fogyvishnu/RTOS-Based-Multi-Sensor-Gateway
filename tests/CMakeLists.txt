cmake_minimum_required(VERSION 3.10)
project(SensorGatewayHostTests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

include_directories(
    ../App/Inc
    ../Middleware/RingBuffer
)

# Test 1: Sensor Fusion
add_executable(test_sensor_fusion
    test_sensor_fusion.c
    ../App/Src/sensor_fusion.c
)
target_link_libraries(test_sensor_fusion m)

# Test 2: Fault Manager
add_executable(test_fault_manager
    test_fault_manager.c
    ../App/Src/fault_manager.c
)
target_link_libraries(test_fault_manager m)

enable_testing()
add_test(NAME SensorFusionTest COMMAND test_sensor_fusion)
add_test(NAME FaultManagerTest COMMAND test_fault_manager)
