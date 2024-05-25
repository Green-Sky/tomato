cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET SDL3::SDL3)
	set(SDL_SHARED OFF CACHE INTERNAL "")
	set(SDL_STATIC ON CACHE INTERNAL "")
	#TODO: pic ?

	FetchContent_Declare(SDL3
		#[[
		GIT_REPOSITORY https://github.com/libsdl-org/SDL
		#GIT_TAG 0429f5d6a36fc35b551bcc2acd4a40c2db6dab82 # tip when looking
		#GIT_TAG 14f584a94bfd49cf1524db75bf3c419fdf9436cd # tip 26-04-2024
		GIT_TAG 06d6f2cb2518622593570985589700910cf4399f # 13-05-2024 - before
		# made changes that break imgui release 1.90.6
		#GIT_TAG 1103294d33f47ab4c697bb22a9cf27c79c658630 # tip 15-05-2024
		]]

		GIT_REPOSITORY https://github.com/Green-Sky/SDL.git
		GIT_TAG windows_clipboard_fixes
		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3)
endif()

