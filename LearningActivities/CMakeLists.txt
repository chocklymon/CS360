cmake_minimum_required(VERSION 3.3)
project(LearningActivities)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES sigint.c stat.c threads.c client.c fork.cpp)
add_executable(threads threads.c)
add_executable(sigint sigint.c)
add_executable(stat stat.c)
add_executable(sem semaphores.cpp)
add_executable(epoll client.c)
add_executable(fork fork.cpp)
