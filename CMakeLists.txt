cmake_minimum_required(VERSION 3.15)
project(SMART_TRAFFIC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(CORE_SOURCES
    cpu_scheduler.cpp
    traffic_scheduler.cpp
    metrics.cpp
    simulation.cpp
)

# Main executable
add_executable(smart_traffic main.cpp ${CORE_SOURCES})

# Test executable
add_executable(run_tests tests.cpp ${CORE_SOURCES})
