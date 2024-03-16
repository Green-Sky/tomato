cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET TracyClient)
	FetchContent_Declare(tracy
		GIT_REPOSITORY https://github.com/wolfpld/tracy.git
		GIT_TAG v0.10
	)
	FetchContent_MakeAvailable(tracy)
endif()

