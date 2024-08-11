cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

add_library(SPSCQueue INTERFACE
	./SPSCQueue.h
)

target_compile_features(SPSCQueue INTERFACE cxx_std_17)
target_include_directories(SPSCQueue INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

