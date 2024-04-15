cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET SDL3_image::SDL3_image)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	# TODO: make pr to make this an option
	set(SDL3IMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(SDL3IMAGE_BACKEND_STB OFF CACHE BOOL "" FORCE) # important for security
	set(SDL3IMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE) # some funky apple cmake bug
	set(SDL3IMAGE_JXL ON CACHE BOOL "" FORCE)
	set(SDL3IMAGE_QOI OFF CACHE BOOL "" FORCE) # we have our own

	FetchContent_Declare(SDL3_image
		GIT_REPOSITORY https://github.com/libsdl-org/SDL_image
		# waiting on the imgui pr to get merged so i can update sdl <.<
		GIT_TAG a45d6e5b84ccc0f3faae6ba7d561709ed600eee7 # tip last check
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3_image)
endif()

