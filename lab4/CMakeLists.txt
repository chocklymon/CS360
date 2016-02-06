cmake_minimum_required(VERSION 3.3)
project(lab4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        webtest.c)

add_executable(lab4 ${SOURCE_FILES})