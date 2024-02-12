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

