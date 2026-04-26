cmake_minimum_required(VERSION 3.10)
project(RPCClient C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find ZMQ
pkg_check_modules(ZMQ REQUIRED libzmq)
if(NOT ZMQ_FOUND)
    message(FATAL_ERROR "libzmq not found. Install with: sudo apt-get install libzmq3-dev")
endif()

# Find libevent
pkg_check_modules(EVENT REQUIRED libevent)
if(NOT EVENT_FOUND)
    message(FATAL_ERROR "libevent not found. Install with: sudo apt-get install libevent-dev")
endif()

# Find yyjson
pkg_check_modules(YYJSON REQUIRED yyjson)
if(NOT YYJSON_FOUND)
    message(WARNING "yyjson not found via pkg-config. Attempting to find manually...")
    find_path(YYJSON_INCLUDE_DIR yyjson.h)
    find_library(YYJSON_LIBRARY NAMES yyjson)
    if(YYJSON_INCLUDE_DIR AND YYJSON_LIBRARY)
        set(YYJSON_INCLUDE_DIRS ${YYJSON_INCLUDE_DIR})
        set(YYJSON_LIBRARIES ${YYJSON_LIBRARY})
    else()
        message(FATAL_ERROR "yyjson not found. Install from: https://github.com/ibireme/yyjson")
    endif()
endif()

# Source files
set(SOURCES
    main.c
    rpc_client.c
)

# Create executable
add_executable(rpc_client ${SOURCES})

# Include directories
target_include_directories(rpc_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZMQ_INCLUDE_DIRS}
    ${EVENT_INCLUDE_DIRS}
    ${YYJSON_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(rpc_client
    ${ZMQ_LIBRARIES}
    ${EVENT_LIBRARIES}
    ${YYJSON_LIBRARIES}
)

# Compiler flags
target_compile_options(rpc_client PRIVATE
    -Wall
    -Wextra
    -O2
)

# Print configuration info
message(STATUS "ZMQ found: ${ZMQ_LIBRARIES}")
message(STATUS "Event found: ${EVENT_LIBRARIES}")
message(STATUS "yyjson found: ${YYJSON_LIBRARIES}")
