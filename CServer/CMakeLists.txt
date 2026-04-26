cmake_minimum_required(VERSION 3.10)
project(RPCServer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find ZMQ
pkg_check_modules(ZMQ REQUIRED libzmq)
if(NOT ZMQ_FOUND)
    message(FATAL_ERROR "libzmq not found. Install with: sudo apt-get install libzmq3-dev")
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

# Find pthread
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    main.c
    rpc_server.c
)

# Create executable
add_executable(rpc_server ${SOURCES})

# Include directories
target_include_directories(rpc_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZMQ_INCLUDE_DIRS}
    ${YYJSON_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(rpc_server
    ${ZMQ_LIBRARIES}
    ${YYJSON_LIBRARIES}
    Threads::Threads
)

# Compiler flags
target_compile_options(rpc_server PRIVATE
    -Wall
    -Wextra
    -O2
)

# Print configuration info
message(STATUS "ZMQ found: ${ZMQ_LIBRARIES}")
message(STATUS "yyjson found: ${YYJSON_LIBRARIES}")
message(STATUS "Threads found: ${CMAKE_THREAD_LIBS_INIT}")
