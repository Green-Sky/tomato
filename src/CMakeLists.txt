cmake_minimum_required(VERSION 3.9...3.24 FATAL_ERROR)

########################################

if (TOMATO_MAIN_SO)
	add_library(tomato MODULE)
	target_compile_definitions(tomato PUBLIC TOMATO_MAIN_SO)
else()
	add_executable(tomato)
endif()

target_sources(tomato PUBLIC
	./main.cpp
	./icon.rc

	./sys_check.hpp
	./sys_check.cpp

	./json_to_config.hpp
	./json_to_config.cpp

	./screen.hpp
	./start_screen.hpp
	./start_screen.cpp
	./main_screen.hpp
	./main_screen.cpp

	./tox_client.hpp
	./tox_client.cpp
	./auto_dirty.hpp
	./auto_dirty.cpp
	./tox_private_impl.hpp

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
	./image_loader_sdl_image.hpp
	./image_loader_sdl_image.cpp

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

	./chat_gui/theme.hpp
	./chat_gui/theme.cpp
	./chat_gui/icons/direct.hpp
	./chat_gui/icons/direct.cpp
	./chat_gui/icons/cloud.hpp
	./chat_gui/icons/cloud.cpp
	./chat_gui/icons/mail.hpp
	./chat_gui/icons/mail.cpp
	./chat_gui/icons/person.hpp
	./chat_gui/icons/person.cpp
	./chat_gui/icons/group.hpp
	./chat_gui/icons/group.cpp
	./chat_gui/contact_list.hpp
	./chat_gui/contact_list.cpp
	./chat_gui/file_selector.hpp
	./chat_gui/file_selector.cpp
	./chat_gui/send_image_popup.hpp
	./chat_gui/send_image_popup.cpp
	./chat_gui/settings_window.hpp
	./chat_gui/settings_window.cpp

	./imgui_entt_entity_editor.hpp
	./object_store_ui.hpp
	./object_store_ui.cpp

	./tox_ui_utils.hpp
	./tox_ui_utils.cpp

	./tox_dht_cap_histo.hpp
	./tox_dht_cap_histo.cpp

	./tox_friend_faux_offline_messaging.hpp
	./tox_friend_faux_offline_messaging.cpp

	./chat_gui4.hpp
	./chat_gui4.cpp

	./frame_streams/frame_stream2.hpp
	./frame_streams/audio_stream2.hpp
	./frame_streams/stream_manager.hpp
	./frame_streams/stream_manager.cpp
)

if (TOMATO_TOX_AV)
	target_sources(tomato PUBLIC
		./tox_av.hpp
		./tox_av.cpp
	)

	target_compile_definitions(tomato PUBLIC TOMATO_TOX_AV)
endif()

target_compile_features(tomato PUBLIC cxx_std_17)
target_link_libraries(tomato PUBLIC
	solanaceae_util
	solanaceae_contact
	solanaceae_message3
	solanaceae_message_serializer

	solanaceae_plugin

	solanaceae_toxcore
	solanaceae_tox_contacts
	solanaceae_tox_messages

	solanaceae_object_store

	SDL3::SDL3

	imgui
	imgui_backend_sdl3
	imgui_backend_sdlrenderer3

	stb_image
	stb_image_write
	webpdemux
	libwebpmux # the f why (needed for anim encode)
	qoi
	SDL3_image::SDL3_image
)

# probably not enough
#target_compile_definitions(tomato PUBLIC ENTT_API_EXPORT)

set_target_properties(tomato PROPERTIES POSITION_INDEPENDENT_CODE ON)

