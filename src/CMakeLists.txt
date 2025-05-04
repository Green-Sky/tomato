cmake_minimum_required(VERSION 3.9...3.24 FATAL_ERROR)

########################################

if (TOMATO_MAIN_SO)
	add_library(tomato MODULE)
	target_compile_definitions(tomato PUBLIC TOMATO_MAIN_SO)
else()
	add_executable(tomato)

	install(TARGETS tomato)
endif()

target_sources(tomato PUBLIC
	./main.cpp
	./icon.rc

	./icon_generator.hpp
	./icon_generator.cpp

	./sys_check.hpp
	./sys_check.cpp

	./json_to_config.hpp
	./json_to_config.cpp

	./screen.hpp
	./start_screen.hpp
	./start_screen.cpp
	./main_screen.hpp
	./main_screen.cpp

	./backends/std_fs.hpp
	./backends/std_fs.cpp

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
	./bitset_image_loader.hpp
	./bitset_image_loader.cpp

	./tox_avatar_manager.hpp
	./tox_avatar_manager.cpp

	./media_meta_info_loader.hpp
	./media_meta_info_loader.cpp

	./sdl_clipboard_utils.hpp
	./sdl_clipboard_utils.cpp

	./sys_tray.hpp
	./sys_tray.cpp

	./string_formatter_utils.hpp
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
	./chat_gui/contact_list_sorter.hpp
	./chat_gui/contact_list_sorter.cpp
	./chat_gui/file_selector.hpp
	./chat_gui/file_selector.cpp
	./chat_gui/image_viewer_popup.hpp
	./chat_gui/image_viewer_popup.cpp
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

	./tox_netprof_ui.hpp
	./tox_netprof_ui.cpp

	./tox_friend_faux_offline_messaging.hpp
	./tox_friend_faux_offline_messaging.cpp

	./status_indicator.hpp
	./status_indicator.cpp

	./chat_gui4.hpp
	./chat_gui4.cpp

	./frame_streams/frame_stream2.hpp
	./frame_streams/audio_stream2.hpp
	./frame_streams/stream_manager.hpp
	./frame_streams/stream_manager.cpp
	./frame_streams/locked_frame_stream.hpp
	./frame_streams/multi_source.hpp

	./frame_streams/voip_model.hpp

	./frame_streams/sdl/sdl_audio2_frame_stream2.hpp
	./frame_streams/sdl/sdl_audio2_frame_stream2.cpp
	./frame_streams/sdl/video.hpp
	./frame_streams/sdl/video_push_converter.hpp
	./frame_streams/sdl/sdl_video_frame_stream2.hpp
	./frame_streams/sdl/sdl_video_frame_stream2.cpp
	./frame_streams/sdl/sdl_video_input_service.hpp
	./frame_streams/sdl/sdl_video_input_service.cpp

	./stream_manager_ui.hpp
	./stream_manager_ui.cpp

	./debug_video_tap.hpp
	./debug_video_tap.cpp
)

if (TOMATO_BREAKPAD)
	target_sources(tomato PUBLIC
		./breakpad_client.hpp
		./breakpad_client.cpp
	)

	target_link_libraries(tomato PUBLIC breakpad_client)
	target_compile_definitions(tomato PUBLIC TOMATO_BREAKPAD)
endif()

if (TOMATO_TOX_AV)
	target_sources(tomato PUBLIC
		./tox_av.hpp
		./tox_av.cpp

		./tox_av_voip_model.hpp
		./tox_av_voip_model.cpp
	)

	target_compile_definitions(tomato PUBLIC TOMATO_TOX_AV)
endif()

configure_file(./version.hpp.in version.hpp @ONLY)
target_include_directories(tomato PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_features(tomato PUBLIC cxx_std_17)
target_link_libraries(tomato PUBLIC
	solanaceae_util
	solanaceae_contact
	solanaceae_contact_impl
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
	implot

	stb_image
	stb_image_write
	WebP::webp
	WebP::webpdemux
	WebP::libwebpmux # the f why (needed for anim encode)
	qoi
	SDL3_image::SDL3_image
)

set_target_properties(tomato PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(tomato PROPERTIES VERSION "${tomato_VERSION_MAJOR}.${tomato_VERSION_MINOR}.${tomato_VERSION_PATCH}-${TOMATO_GIT_DEPTH}+git.${TOMATO_GIT_COMMIT}")

########################################

add_executable(test_frame_stream2_pop_reframer EXCLUDE_FROM_ALL
	./frame_streams/frame_stream2.hpp
	./frame_streams/audio_stream2.hpp
	./frame_streams/locked_frame_stream.hpp
	./frame_streams/multi_source.hpp

	./frame_streams/test_pop_reframer.cpp
)

target_link_libraries(test_frame_stream2_pop_reframer
	solanaceae_util
)

