cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include(FetchContent)

#makes no sense without freetype
if (NOT TARGET plutosvg::plutosvg AND TARGET Freetype::Freetype)
	set(PLUTOSVG_ENABLE_FREETYPE ON CACHE INTERNAL "")
	set(PLUTOSVG_BUILD_EXAMPLES OFF CACHE INTERNAL "")
	FetchContent_Declare(plutosvg
		GIT_REPOSITORY https://github.com/sammycage/plutosvg.git
		GIT_TAG v0.0.6
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(plutosvg)
endif()

