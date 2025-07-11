cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

# cmake setup begin
project(tomato
	VERSION 0.3.2
	HOMEPAGE_URL https://github.com/Green-Sky/tomato
)

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
option(TOMATO_BREAKPAD "Build tomato with breakpad crash dumping" OFF)
option(TOMATO_TOX_AV "Build tomato with ToxAV" OFF)


if (TOMATO_ASAN)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		if (NOT WIN32) # exclude mingw
			add_compile_options(-fno-omit-frame-pointer)
			add_compile_options(-fsanitize=address,undefined)
			#add_compile_options(-fsanitize=address,undefined,pointer-compare,pointer-subtract)
			#add_compile_options(-fhardened)
			#add_compile_options(-D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS -ftrivial-auto-var-init=zero -fPIE  -pie  -Wl,-z,relro,-z,now -fstack-protector-strong -fstack-clash-protection -fcf-protection=full)

			add_link_options(-fno-omit-frame-pointer)
			add_link_options(-fsanitize=address,undefined)
			#add_link_options(-fsanitize=address,undefined,pointer-compare,pointer-subtract)
			#add_link_options(-fhardened)
			#add_link_options(-D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS -ftrivial-auto-var-init=zero -fPIE  -pie  -Wl,-z,relro,-z,now -fstack-protector-strong -fstack-clash-protection -fcf-protection=full)

			link_libraries(-static-libasan) # make it "work" on nix
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

message("II TOMATO_BREAKPAD: ${TOMATO_BREAKPAD}")
if (TOMATO_BREAKPAD)
	if (LINUX) # TODO: test if android
		# HACK: workaround an ugly cmake bug,
		# where subdirs can now propergate enable_language upwards
		enable_language(ASM)
	endif()
endif()

message("II TOMATO_TOX_AV: ${TOMATO_TOX_AV}")

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

# TODO: move to cmake include (maybe function)
set(TOMATO_GIT_DEPTH 0)
set(TOMATO_GIT_COMMIT "UNK")

find_package(Git QUIET)
if(NOT Git_FOUND)
    find_program(GIT_EXECUTABLE NAMES git git.exe)
    if(GIT_EXECUTABLE)
        set(Git_FOUND TRUE)
        message(STATUS "Found Git: ${GIT_EXECUTABLE}")
    endif()
endif()

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HEAD
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RES
    )
    if (RES EQUAL 0)
		set(TOMATO_GIT_COMMIT ${GIT_HEAD})
    endif()
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_DEPTH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE RES
    )
    if (RES EQUAL 0)
		set(TOMATO_GIT_DEPTH ${GIT_DEPTH})
    endif()
endif()

message(STATUS "tomato version: ")
message(STATUS "${tomato_VERSION_MAJOR}.${tomato_VERSION_MINOR}.${tomato_VERSION_PATCH}-${TOMATO_GIT_DEPTH}+git.${TOMATO_GIT_COMMIT}")

# cmake setup end

if (BUILD_TESTING)
	include(CTest)
endif()

add_subdirectory(./src)

# TODO: move to src
if (ANDROID AND TARGET SDL3::Jar)
	message("II building for ANDROID!!!")

	add_subdirectory(android)
endif()

