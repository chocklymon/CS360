cmake_minimum_required(VERSION 3.3)
project(CS_360_Lab_1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES download.c)
add_executable(CS_360_Lab_1 ${SOURCE_FILES})