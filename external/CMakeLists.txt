cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

add_subdirectory(./entt)

add_subdirectory(./solanaceae_util)
add_subdirectory(./solanaceae_contact)
add_subdirectory(./solanaceae_message3)

add_subdirectory(./solanaceae_plugin)

add_subdirectory(./toxcore)
add_subdirectory(./solanaceae_toxcore)
add_subdirectory(./solanaceae_tox)

add_subdirectory(./sdl)
add_subdirectory(./imgui)

add_subdirectory(./stb)
add_subdirectory(./libwebp)
add_subdirectory(./qoi)

if (NOT TARGET nlohmann_json::nlohmann_json)
	FetchContent_Declare(json
		URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
		URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(json)
endif()

if (NOT TARGET zstd::zstd)
	# TODO: try find_package() first
	# TODO: try pkg-config next (will work on most distros)

	set(ZSTD_BUILD_STATIC ON)
	set(ZSTD_BUILD_SHARED OFF)
	set(ZSTD_BUILD_PROGRAMS OFF)
	set(ZSTD_BUILD_CONTRIB OFF)
	set(ZSTD_BUILD_TESTS OFF)
	FetchContent_Declare(zstd
		URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		SOURCE_SUBDIR build/cmake
		EXCLUDE_FROM_ALL
	)
	FetchContent_MakeAvailable(zstd)

	add_library(zstd INTERFACE) # somehow zstd fkd this up
	target_include_directories(zstd INTERFACE ${zstd_SOURCE_DIR}/lib/)
	target_link_libraries(zstd INTERFACE libzstd_static)
	add_library(zstd::zstd ALIAS zstd)
endif()
