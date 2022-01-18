cmake_minimum_required(VERSION 3.10)
project(serial-port-control)

set(DEBUG           false)
set(SAVE_DATAset    false)
set(SAVE_IMAGE      false)
set(COUNT_TIME      false)

set(CMAKE_CXX_STANDARD 14)

set(BIN_NAME "run")
set(LIB_NAME "comm")

file(GLOB_RECURSE sourcefiles "src/*.cpp")

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/include)

if(DEBUG)
    add_executable(${BIN_NAME} debug.cpp ${sourcefiles})
    target_link_libraries(${BIN_NAME} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${BIN_NAME} ${OpenCV_LIBS})
else()
    ADD_LIBRARY(${LIB_NAME} ${sourcefiles})
    target_link_libraries(${LIB_NAME} ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(${LIB_NAME} ${OpenCV_LIBS})
endif()


