cmake_minimum_required(VERSION 3.3)
project(LearningActivities)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES utils.c http.c server.c)
add_executable(lab3 ${SOURCE_FILES})
