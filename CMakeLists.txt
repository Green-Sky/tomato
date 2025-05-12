cmake_minimum_required(VERSION 3.10)

project(libqoirdo)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

message( ${PROJECT_NAME} " build type: " ${CMAKE_BUILD_TYPE} )

add_library(qoirdo
	./qoirdo.hpp
	./qoirdo.cpp
)

target_compile_features(qoirdo PUBLIC cxx_std_11)

#if (NOT MSVC)
#   target_link_libraries(rdopng m pthread)
#endif()

########################################

add_executable(qoirdo_tool
	tool.cpp
)

target_link_libraries(qoirdo_tool qoirdo)

