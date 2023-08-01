cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(stb C CXX)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(stb_image "stb/stb_image.h" "stb_image.cpp")
target_link_libraries(stb_image stb)

add_library(stb_image_write "stb/stb_image_write.h" "stb_image_write.cpp")
target_link_libraries(stb_image_write stb)

