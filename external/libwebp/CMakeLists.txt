cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

option(TOMATO_DEP_LIBWEBP_USE_SYSTEM "Prefer system provided libwebp" OFF)

if (TOMATO_DEP_LIBWEBP_USE_SYSTEM)
	if (NOT TARGET WebP::libwebpmux)
		cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
		find_package(WebP CONFIG GLOBAL QUIET COMPONENTS webp webpdemux libwebpmux)
	endif()

	#if (NOT TARGET libwebp)
	#    find_package(WebP GLOBAL)
	#endif()

	if (TARGET WebP::webp)
		message(STATUS "found WebP::webp")
	endif()
	if (TARGET WebP::webpdemux)
		message(STATUS "found WebP::webpdemux")
	endif()
	if (TARGET WebP::libwebpmux)
		message(STATUS "found WebP::libwebpmux")
	endif()

	if (TARGET WebP::libwebpmux)
		message(STATUS "found system libwebp")
	else()
		message(STATUS "system libwebp not found")
	endif()
endif()

if (NOT TARGET WebP::libwebpmux)
	set(WEBP_LINK_STATIC ON CACHE BOOL "" FORCE)
	set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_CWEBP OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_DWEBP OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_VWEBP OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_LIBWEBPMUX ON CACHE BOOL "" FORCE)
	set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
	set(WEBP_BUILD_WEBP_JS OFF CACHE BOOL "" FORCE)
	set(WEBP_USE_THREAD ON CACHE BOOL "" FORCE)

	FetchContent_Declare(libwebp
		GIT_REPOSITORY https://github.com/webmproject/libwebp
		GIT_TAG v1.5.0
		FIND_PACKAGE_ARGS
	)
	FetchContent_MakeAvailable(libwebp)

	add_library(WebP::webp ALIAS webp)
	add_library(WebP::webpdemux ALIAS webpdemux)
	add_library(WebP::libwebpmux ALIAS libwebpmux)
endif()

