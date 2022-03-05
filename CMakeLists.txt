cmake_minimum_required(VERSION 3.10)
project(serial-port-control)

set(DEBUG           false)
set(OPTIMIZATION    true)

set(CMAKE_CXX_STANDARD 14)

set(BIN_NAME "run")
set(LIB_NAME "comm")

file(GLOB_RECURSE sourcefiles "src/*.cpp")

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

if(DEBUG)
    add_executable(${BIN_NAME} debug.cpp ${sourcefiles})
    target_link_libraries(${BIN_NAME} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${BIN_NAME} Threads::Threads)
else()
    ADD_LIBRARY(${LIB_NAME} ${sourcefiles})
    target_link_libraries(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${LIB_NAME} Threads::Threads)
endif()

IF(OPTIMIZATION)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
ENDIF()
