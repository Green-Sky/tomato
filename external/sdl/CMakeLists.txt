cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

if (NOT TARGET SDL3::SDL3)
	set(SDL_SHARED OFF CACHE INTERNAL "")
	set(SDL_STATIC ON CACHE INTERNAL "")
	#TODO: pic ?
	set(SDL_DISABLE_ANDROID_JAR OFF CACHE INTERNAL "")

	FetchContent_Declare(SDL3
		GIT_REPOSITORY https://github.com/libsdl-org/SDL
		#GIT_TAG 0429f5d6a36fc35b551bcc2acd4a40c2db6dab82 # tip when looking
		#GIT_TAG 14f584a94bfd49cf1524db75bf3c419fdf9436cd # tip 26-04-2024
		#GIT_TAG 06d6f2cb2518622593570985589700910cf4399f # 13-05-2024 - before
		# made changes that break imgui release 1.90.6
		#GIT_TAG 1103294d33f47ab4c697bb22a9cf27c79c658630 # tip 15-05-2024
		#GIT_TAG aacafd62336363077470f678b6217214b3b49473 # tip 28-05-2024
		#GIT_TAG 5fa9432b7d1c1722de93e1ab46e7a9569a47071e # tip 27-05-2024 - before changes made breaking sdl_image
		#GIT_TAG 9651ca59187c16079846918483c40d6b5c2f454c # tip 09-06-2024
		#GIT_TAG 657c0135b1ff1685afa1bad63b0417d92f4bcb46 # tip 09-06-2024
		#GIT_TAG d65a8146b950abe31b4fbf779f3e2fea731af9bd # tip 16-07-2024 - before string policy changes breaking imgui
		#GIT_TAG e949f12f63cdfcef4bdf456936ee676e0a3f9de6 # tip 18-07-2024 - broke bmp surface (before too)
		#GIT_TAG 67b973b5fad633b3be76d4daf4fd9fece292c25f # tip 29-07-2024
		#GIT_TAG 6e885d96193a4b0096fe7fed6d4e6c3e5f247283 # tip 09-09-2024
		GIT_TAG 9dd8859240703d886941733ad32c1dc6f50d64f0 # tip 19-09-2024

		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3)
endif()

