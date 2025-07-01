cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET Freetype::Freetype)
	#makes no sense without freetype
	return()
endif()

option(TOMATO_DEP_PLUTOSVG_USE_SYSTEM "Prefer system provided plutosvg" OFF)

if (TOMATO_DEP_PLUTOSVG_USE_SYSTEM)
	if (NOT TARGET plutosvg::plutosvg)
		cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
		find_package(plutosvg 0.0.6 CONFIG GLOBAL QUIET)
	endif()

	if (TARGET plutosvg::plutosvg)
		message(STATUS "found system plutosvg")
	else()
		message(STATUS "system plutosvg not found")
	endif()

endif()

if (NOT TARGET plutosvg::plutosvg)
	set(PLUTOSVG_ENABLE_FREETYPE ON CACHE INTERNAL "")
	set(PLUTOSVG_BUILD_EXAMPLES OFF CACHE INTERNAL "")
	FetchContent_Declare(plutosvg
		GIT_REPOSITORY https://github.com/sammycage/plutosvg.git
		GIT_TAG v0.0.7
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(plutosvg)
endif()

