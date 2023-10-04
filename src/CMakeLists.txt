cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

add_executable(tomato
	./main.cpp
	./icon.rc

	./screen.hpp
	./start_screen.hpp
	./start_screen.cpp
	./main_screen.hpp
	./main_screen.cpp

	./tox_client.hpp
	./tox_client.cpp
	./auto_dirty.hpp
	./auto_dirty.cpp

	./theme.hpp

	./image_loader.hpp
	./image_loader_sdl_bmp.hpp
	./image_loader_sdl_bmp.cpp
	./image_loader_stb.hpp
	./image_loader_stb.cpp
	./image_loader_webp.hpp
	./image_loader_webp.cpp

	./texture_uploader.hpp
	./sdlrenderer_texture_uploader.hpp
	./sdlrenderer_texture_uploader.cpp

	./texture_cache.hpp
	./texture_cache.cpp
	./tox_avatar_loader.hpp
	./tox_avatar_loader.cpp
	./message_image_loader.hpp
	./message_image_loader.cpp

	./media_meta_info_loader.hpp
	./media_meta_info_loader.cpp

	./sdl_clipboard_utils.hpp
	./sdl_clipboard_utils.cpp

	./file_selector.hpp
	./file_selector.cpp

	./send_image_popup.hpp
	./send_image_popup.cpp

	./chat_gui4.hpp
	./chat_gui4.cpp
)

target_compile_features(tomato PUBLIC cxx_std_17)
target_link_libraries(tomato PUBLIC
	solanaceae_util
	solanaceae_contact
	solanaceae_message3

	solanaceae_plugin

	solanaceae_toxcore
	solanaceae_tox_contacts
	solanaceae_tox_messages

	SDL3::SDL3

	imgui
	imgui_backend_sdl3
	imgui_backend_sdlrenderer3

	stb_image
	webpdemux
	libwebpmux # the f why (needed for anim encode)
)

