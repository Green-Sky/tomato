cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET SDL3::SDL3)
	set(SDL_SHARED OFF CACHE INTERNAL "")
	set(SDL_STATIC ON CACHE INTERNAL "")
	#TODO: pic ?

	FetchContent_Declare(SDL3
		GIT_REPOSITORY https://github.com/libsdl-org/SDL
		#GIT_TAG 0429f5d6a36fc35b551bcc2acd4a40c2db6dab82 # tip when looking
		GIT_TAG 14f584a94bfd49cf1524db75bf3c419fdf9436cd # tip 26-04-2024
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3)
endif()

