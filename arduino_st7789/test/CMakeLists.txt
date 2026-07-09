cmake_minimum_required(VERSION 3.10)
project(st7789)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(st7789
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/tft.cpp
)
target_include_directories(st7789 PRIVATE ${SRC_DIR})
target_compile_features(st7789 PRIVATE cxx_std_20)
target_compile_definitions(st7789 PRIVATE TEST_HARNESS)


add_executable(test_cobs
    ${SRC_DIR}/test_cobs.cpp
)
target_include_directories(test_cobs PRIVATE ${SRC_DIR})
target_compile_features(test_cobs PRIVATE cxx_std_20)

