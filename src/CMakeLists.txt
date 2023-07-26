cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

add_executable(tomato
	main.cpp
	icon.rc

	screen.hpp
	start_screen.hpp
	start_screen.cpp
	main_screen.hpp
	main_screen.cpp
)

target_compile_features(tomato PUBLIC cxx_std_17)
target_link_libraries(tomato PUBLIC
	solanaceae_util
	solanaceae_contact
	solanaceae_message3

	#solanaceae_plugin

	solanaceae_toxcore
	solanaceae_tox_contacts
	solanaceae_tox_messages

	SDL3::SDL3

	imgui
	imgui_backend_sdl3
	imgui_backend_sdlrenderer3
)

