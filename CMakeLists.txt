project(sunnet)

cmake_minimum_required(VERSION 2.8)

include_directories(include)

aux_source_directory(./src DIR_SRCS)

add_definitions(-std=c++11)

add_executable(sunnet ${DIR_SRCS})

find_package(Threads REQUIRED)
target_link_libraries(sunnet ${CMAKE_THREAD_LIBS_INIT})
