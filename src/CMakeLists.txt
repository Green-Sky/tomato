cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

add_library(fragment_store
	./fragment_store/fragment_store_i.hpp
	./fragment_store/fragment_store_i.cpp
	./fragment_store/types.hpp
	./fragment_store/meta_components.hpp
	./fragment_store/meta_components_id.inl
	./fragment_store/serializer.hpp
	./fragment_store/fragment_store.hpp
	./fragment_store/fragment_store.cpp

	./json/message_components.hpp # TODO: move
	./json/tox_message_components.hpp # TODO: move
)

target_link_libraries(fragment_store PUBLIC
	nlohmann_json::nlohmann_json
	EnTT::EnTT
	solanaceae_util

	zstd::zstd

	solanaceae_tox_messages # TODO: move
)

########################################

add_library(message_fragment_store
	./fragment_store/message_serializer.hpp
	./fragment_store/message_serializer.cpp
	./fragment_store/message_fragment_store.hpp
	./fragment_store/message_fragment_store.cpp

	./fragment_store/register_mfs_json_message_components.hpp
	./fragment_store/register_mfs_json_message_components.cpp
	./fragment_store/register_mfs_json_tox_message_components.hpp
	./fragment_store/register_mfs_json_tox_message_components.cpp
)
target_compile_features(message_fragment_store PRIVATE cxx_std_20)
target_link_libraries(message_fragment_store PUBLIC
	fragment_store
	solanaceae_message3
)

########################################

add_executable(fragment_store_test
	fragment_store/test_fragstore.cpp
)

target_link_libraries(fragment_store_test PUBLIC
	fragment_store
)

########################################

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
	./image_loader.cpp
	./image_loader_sdl_bmp.hpp
	./image_loader_sdl_bmp.cpp
	./image_loader_stb.hpp
	./image_loader_stb.cpp
	./image_loader_webp.hpp
	./image_loader_webp.cpp
	./image_loader_qoi.hpp
	./image_loader_qoi.cpp

	./texture_uploader.hpp
	./sdlrenderer_texture_uploader.hpp
	./sdlrenderer_texture_uploader.cpp

	./texture_cache.hpp
	./texture_cache.cpp
	./tox_avatar_loader.hpp
	./tox_avatar_loader.cpp
	./message_image_loader.hpp
	./message_image_loader.cpp

	./tox_avatar_manager.hpp
	./tox_avatar_manager.cpp

	./media_meta_info_loader.hpp
	./media_meta_info_loader.cpp

	./sdl_clipboard_utils.hpp
	./sdl_clipboard_utils.cpp

	./file_selector.hpp
	./file_selector.cpp

	./send_image_popup.hpp
	./send_image_popup.cpp

	./settings_window.hpp
	./settings_window.cpp

	./tox_ui_utils.hpp
	./tox_ui_utils.cpp

	./tox_dht_cap_histo.hpp
	./tox_dht_cap_histo.cpp

	./tox_friend_faux_offline_messaging.hpp
	./tox_friend_faux_offline_messaging.cpp

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

	fragment_store
	message_fragment_store

	SDL3::SDL3

	imgui
	imgui_backend_sdl3
	imgui_backend_sdlrenderer3

	stb_image
	stb_image_write
	webpdemux
	libwebpmux # the f why (needed for anim encode)
	qoi
)

