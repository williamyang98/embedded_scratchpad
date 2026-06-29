cmake_minimum_required(VERSION 3.10)
project(st7789)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(st7789 ${SRC_DIR}/main.cpp)
target_include_directories(st7789 PRIVATE ${SRC_DIR})
target_compile_features(st7789 PRIVATE cxx_std_20)

