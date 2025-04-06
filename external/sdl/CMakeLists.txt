cmake_minimum_required(VERSION 3.16...3.24 FATAL_ERROR)

include(FetchContent)

option(TOMATO_DEP_SDL_USE_SYSTEM "Prefer system provided SDL" OFF)

if (TOMATO_DEP_SDL_USE_SYSTEM)
	if (NOT TARGET SDL3::SDL3)
		#find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
		find_package(SDL3 REQUIRED CONFIG GLOBAL QUIET)
	endif()

	if (TARGET SDL3::SDL3)
		message(STATUS "found system SDL3")
	else()
		message(STATUS "system SDL3 not found")
	endif()
endif()

if (NOT TARGET SDL3::SDL3)
	message(STATUS "using bundled SDL3")
	# bundle sdl static
	set(SDL_SHARED OFF CACHE INTERNAL "")
	set(SDL_STATIC ON CACHE INTERNAL "")
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
		#GIT_TAG 9dd8859240703d886941733ad32c1dc6f50d64f0 # tip 19-09-2024
		#GIT_TAG afdf325fb4090e93a124519d1a3bc1fbe0ba9025 # bad
		#GIT_TAG e292d1f5ace469f718d7b6b4dec8c28e37dcaa0e # tip 05-10-2024 (3.1.3)
		#GIT_TAG 2654d5d48b8f764148a7c246fea85b32b1133578 # tip 18-10-2024
		#GIT_TAG f8468d580d903e106640800034a4721aca24264c # tip 15-11-2024
		#GIT_TAG f40a530fd9fcdea79986de12d305426c0304d60e # tip 24-12-2024 - systray yay
		#GIT_TAG f8f8d87bfaf3d8557a5c13e0072964a537569b6c # tip 02-01-2025 - android cam fix
		#GIT_TAG 22422f7748d5128135995ed34c8f8012861c7332 # tip 07-01-2025 (3.1.8)
		#GIT_TAG 191a6417d581b79cab5698dee84554ea64b5ac61 # tip 14-01-2025 - hotplug asan crash fix
		#GIT_TAG 355f69ebfe7a5f44a28e10569589a1d300aad2b5 # tip 15-01-2025 - more recent for continous tray testing
		#GIT_TAG 90b2e2527eeb6614cedd15f61062f62680252be6 # tip 20-01-2025 - tray supposetly fixed
		#GIT_TAG b5c3eab6b447111d3c7879bb547b80fb4abd9063 # tip 07-02-2025 (3.2.4)
		#GIT_TAG f6864924f76e1a0b4abaefc76ae2ed22b1a8916e # tip 05-03-2025 (3.2.8)
		GIT_TAG 877399b2b2cf21e67554ed9046410f268ce1d1b2 # 3.2.10

		FIND_PACKAGE_ARGS # for the future
	)
	FetchContent_MakeAvailable(SDL3)
endif()

