cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

# cmake setup begin
project(tomato)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# defaulting to debug mode, if not specified
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

# setup my vim ycm :D
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# more paths
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(TOMATO_MAIN_SO "Build tomato as a shared object (for eg android apps)" ANDROID)
option(TOMATO_ASAN "Build tomato with asan (gcc/clang/msvc)" OFF)

if (TOMATO_ASAN)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		if (NOT WIN32) # exclude mingw
			#link_libraries(-fsanitize=address)
			link_libraries(-fsanitize=address,undefined)
			#link_libraries(-fsanitize=undefined)
			message("II enabled ASAN")
		else()
			message("!! can not enable ASAN on this platform (gcc/clang + win)")
		endif()
	elseif (MSVC)
		add_compile_options("/fsanitize=address")
		message("II enabled ASAN")
	else()
		message("!! can not enable ASAN on this platform")
	endif()
endif()

# uggly, but it needs to be defined for all of tomato.
# but this also means that we can not compile tomato in the same cmake as plugins
add_compile_definitions(ENTT_API_EXPORT)

# external libs
add_subdirectory(./external EXCLUDE_FROM_ALL) # before increasing warn levels, sad :(

set(CMAKE_CXX_EXTENSIONS OFF)

# bump up warning levels appropriately for clang, gcc & msvc
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	add_compile_options(
		-Wall -Wextra # Reasonable and standard
		-Wpedantic # Warn if non-standard C++ is used
		-Wunused # Warn on anything being unused
		#-Wconversion # Warn on type conversions that may lose data
		#-Wsign-conversion # Warn on sign conversions
		-Wshadow # Warn if a variable declaration shadows one from a parent context
	)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
	if (MSVC)
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()

# cmake setup end

add_subdirectory(./src)

# TODO: move to src
if (ANDROID AND TARGET SDL3::Jar)
	message("II building for ANDROID!!!")

	add_subdirectory(android)
endif()

