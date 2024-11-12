cmake_minimum_required(VERSION 3.14...3.24 FATAL_ERROR)

add_subdirectory(./entt)

add_subdirectory(./json)

add_subdirectory(./solanaceae_util)
add_subdirectory(./solanaceae_object_store)
add_subdirectory(./solanaceae_contact)
add_subdirectory(./solanaceae_message3)
add_subdirectory(./solanaceae_message_serializer)

add_subdirectory(./solanaceae_plugin)

add_subdirectory(./toxcore)
add_subdirectory(./solanaceae_toxcore)
add_subdirectory(./solanaceae_tox)

add_subdirectory(./sdl)
add_subdirectory(./imgui)

add_subdirectory(./stb)
add_subdirectory(./libwebp)
add_subdirectory(./qoi)
add_subdirectory(./sdl_image)

if (TOMATO_BREAKPAD)
	add_subdirectory(./breakpad)
endif()

