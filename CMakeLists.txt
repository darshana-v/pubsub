cmake_minimum_required(VERSION 3.16)
project(pubsub_latency_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
endif()

# Find ZeroMQ
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

# Find cppzmq headers
find_path(CPPZMQ_INCLUDE_DIR zmq.hpp
    PATHS /opt/homebrew/include /usr/local/include /usr/include
    PATH_SUFFIXES cppzmq
)

if(NOT CPPZMQ_INCLUDE_DIR)
    message(FATAL_ERROR "cppzmq headers not found. Install with: brew install cppzmq")
endif()

# UDP Implementation
add_executable(udp_publisher udp/publisher.cpp)
add_executable(udp_subscriber udp/subscriber.cpp)

# Shared Memory Implementation
add_executable(shm_publisher buffer/shm_publisher.cpp)
add_executable(shm_subscriber buffer/shm_subscriber.cpp)

# Improved Shared Memory Implementation
add_executable(shm_publisher_improved buffer/shm_publisher_improved.cpp)
add_executable(shm_subscriber_improved buffer/shm_subscriber_improved.cpp)

# ZeroMQ Implementation
add_executable(zmq_publisher zmq/zmq_publisher.cpp)
add_executable(zmq_subscriber zmq/zmq_subscriber.cpp)

# Link ZeroMQ
target_link_libraries(zmq_publisher ${ZMQ_LIBRARIES})
target_link_libraries(zmq_subscriber ${ZMQ_LIBRARIES})
target_link_directories(zmq_publisher PRIVATE ${ZMQ_LIBRARY_DIRS})
target_link_directories(zmq_subscriber PRIVATE ${ZMQ_LIBRARY_DIRS})

# Include directories for ZeroMQ
target_include_directories(zmq_publisher PRIVATE ${ZMQ_INCLUDE_DIRS} ${CPPZMQ_INCLUDE_DIR})
target_include_directories(zmq_subscriber PRIVATE ${ZMQ_INCLUDE_DIRS} ${CPPZMQ_INCLUDE_DIR})

# Compiler flags for ZeroMQ
target_compile_options(zmq_publisher PRIVATE ${ZMQ_CFLAGS_OTHER})
target_compile_options(zmq_subscriber PRIVATE ${ZMQ_CFLAGS_OTHER})

# Test harness
add_executable(latency_test test_harness.cpp)
target_link_libraries(latency_test ${ZMQ_LIBRARIES})
target_link_directories(latency_test PRIVATE ${ZMQ_LIBRARY_DIRS})
target_include_directories(latency_test PRIVATE ${ZMQ_INCLUDE_DIRS} ${CPPZMQ_INCLUDE_DIR})
target_compile_options(latency_test PRIVATE ${ZMQ_CFLAGS_OTHER})

# Installation
install(TARGETS udp_publisher udp_subscriber 
               shm_publisher shm_subscriber 
               zmq_publisher zmq_subscriber 
               latency_test
        DESTINATION bin)

# Custom targets for running tests
add_custom_target(run_udp_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/udp_subscriber 5555 &
    COMMAND sleep 1
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/udp_publisher 127.0.0.1 5555 1000
    COMMENT "Running UDP latency test"
)

add_custom_target(run_shm_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shm_subscriber test_shm &
    COMMAND sleep 1
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/shm_publisher test_shm 1000
    COMMENT "Running SHM latency test"
)

add_custom_target(run_zmq_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/zmq_subscriber &
    COMMAND sleep 1
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/zmq_publisher 1000
    COMMENT "Running ZeroMQ latency test"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/latency_test
    COMMENT "Running all latency tests"
    DEPENDS latency_test
)
