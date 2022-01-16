cmake_minimum_required(VERSION 3.10)
project(serial_port)

set(CMAKE_CXX_STANDARD 14)

add_executable(serial_port debug.cpp src/SerialPort.cpp src/CommHandle.cpp)
