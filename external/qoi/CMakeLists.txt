cmake_minimum_required(VERSION 3.13...3.24 FATAL_ERROR)

project(qoi C CXX)

add_library(qoi_interface INTERFACE)
target_include_directories(qoi_interface SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

# static lib with impl
add_library(qoi "qoi.cpp")
target_link_libraries(qoi qoi_interface)

