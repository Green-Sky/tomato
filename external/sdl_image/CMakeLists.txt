cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

option(TOMATO_DEP_SDL_IMAGE_USE_SYSTEM "Prefer system provided SDL_image" OFF)

if (TOMATO_DEP_SDL_IMAGE_USE_SYSTEM)
	if (NOT TARGET SDL3_image::SDL3_image)
		cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
		find_package(SDL3_image CONFIG GLOBAL QUIET)
	endif()

	if (TARGET SDL3_image::SDL3_image)
		message(STATUS "found system SDL3_image")
	else()
		message(STATUS "system SDL3_image not found")
	endif()
endif()

if (SDLIMAGE_BACKEND_STB)
	# mandatory security
	# technically its only used if no proper libjpeg and libpng where found/configured
	message(FATAL_ERROR "SDL3_image has the STBI backend enabled")
endif()

if (NOT TARGET SDL3_image::SDL3_image)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	# TODO: make pr to make this an option
	set(SDLIMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(SDLIMAGE_BACKEND_STB OFF CACHE BOOL "" FORCE) # important for security
	set(SDLIMAGE_BACKEND_IMAGEIO OFF CACHE BOOL "" FORCE) # some funky apple cmake bug
	#set(SDLIMAGE_JXL ON CACHE BOOL "" FORCE) # default to off
	set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE) # we have our own
	set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE) # we have our own

	FetchContent_Declare(SDL3_image
		GIT_REPOSITORY https://github.com/libsdl-org/SDL_image
		# waiting on the imgui pr to get merged so i can update sdl <.<
		#GIT_TAG a45d6e5b84ccc0f3faae6ba7d561709ed600eee7 # tip last check
		#GIT_TAG a34ccf16f961e6d5a480045eb650fc3dddb4bfaa # tip 14-05-2024
		#GIT_TAG 2fc5310a9a2700fc856663200f94edebeb5e554a # tip 28-05-2024
		#GIT_TAG 8eff782fa33d795c9ea1ac42dbe7e17cc9874c78 # tip 09-06-2024
		#GIT_TAG 8abc07df88cc035997e797967ac2f479b0e50981 # tip 18-07-2024
		#GIT_TAG 2a27018eda394a4e005cd8ba6bb3bfd0298809c7 # tip 29-07-2024
		#GIT_TAG c1b272450d306db3845086cc231acae736b92f4f # tip 09-09-2024
		#GIT_TAG b56e6c4d1a1c03b3904a8ad21f1fa73c651ffbfc # tip 17-09-2024
		#GIT_TAG 6f4584340b9b78542d11bf0232a1a0862de1f0a9 # tip 25-09-2024
		#GIT_TAG 4ff27afa450eabd2a827e49ed86fab9e3bf826c5 # prerelease-3.1.1
		#GIT_TAG 11154afb7855293159588b245b446a4ef09e574f # release-3.2.4
		#GIT_TAG d78dd65454f442bfdbc373201165f1ef579484ab # release-3.2.6
		#GIT_TAG ad58ecfc27a1bd09e510ceff8bbbdb1094806476 # release-3.4.0
		GIT_TAG bf5131e1bf20bdb1f90d0c9b1558a2db4b052fe1 # tip 16-02-2026
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3_image)
endif()

