project(sunnet)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE Debug)

include_directories(include)

aux_source_directory(./src DIR_SRCS)

# 直接使用 Lua 库的完整路径
set(LUA_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd/lua-5.4.7/src)

add_definitions(-std=c++11)

add_executable(sunnet ${DIR_SRCS})

find_package(Threads REQUIRED)
target_link_libraries(sunnet ${CMAKE_THREAD_LIBS_INIT})

include_directories(./3rd/lua-5.4.7/src)
target_link_libraries(sunnet ${LUA_LIB_DIR}/liblua.a)
target_link_libraries(sunnet dl)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

