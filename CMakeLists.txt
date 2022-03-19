cmake_minimum_required(VERSION 3.10)
project(serial-port-control)

set(DEBUG               true)
set(OPTIMIZATION        false)
set(ABANDON_SAME_FRAME  false)

set(CMAKE_CXX_STANDARD 14)

set(BIN_NAME "run")
set(LIB_NAME "serial")

file(GLOB_RECURSE sourcefiles "src/*.cpp")

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

if(ABANDON_SAME_FRAME)
    add_compile_definitions(ABANDON_SAME_FRAME)
endif()

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
