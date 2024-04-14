cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET libwebp)
	set(WEBP_LINK_STATIC ON CACHE INTERNAL "")
	FetchContent_Declare(libwebp
		GIT_REPOSITORY https://github.com/webmproject/libwebp
		GIT_TAG v1.3.2
		FIND_PACKAGE_ARGS
	)
	FetchContent_MakeAvailable(libwebp)
endif()

#add_subdirectory(./libwebp EXCLUDE_FROM_ALL)

