#pragma once

#include "./screen.hpp"

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/util/simple_config_model.hpp>
#include <solanaceae/contact/contact_store_impl.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/message3/registry_message_model_impl.hpp>
#include <solanaceae/message3/message_time_sort.hpp>
#include <solanaceae/message3/message_serializer.hpp>
#include <solanaceae/plugin/plugin_manager.hpp>
#include <solanaceae/toxcore/tox_event_logger.hpp>
#include "./tox_private_impl.hpp"
#include "./frame_streams/stream_manager.hpp"

#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include <solanaceae/tox_messages/tox_message_manager.hpp>
#include <solanaceae/tox_messages/tox_transfer_manager.hpp>

#include "./tox_client.hpp"
#include "./auto_dirty.hpp"

#include "./media_meta_info_loader.hpp"
#include "./tox_avatar_manager.hpp"
#include "./tox_avatar_sender.hpp"

#include "./sdlrenderer_texture_uploader.hpp"
#include "./texture_cache.hpp"
#include "./chat_gui/texture_cache_defs.hpp"

#include "./sys_tray.hpp"
#include "./status_indicator.hpp"
#include "./chat_gui4.hpp"
#include "./chat_gui/settings_window.hpp"
#include "./object_store_ui.hpp"
#include "./tox_ui_utils.hpp"
#include "./tox_dht_cap_histo.hpp"
#include "./tox_netprof_ui.hpp"
#include "./tox_friend_faux_offline_messaging.hpp"
#include "./frame_streams/sdl/sdl_video_input_service.hpp"
#include "./stream_manager_ui.hpp"
#include "./debug_video_tap.hpp"

#if TOMATO_TOX_AV
#include "./tox_av.hpp"
#include "./tox_av_voip_model.hpp"
#endif

#include <string>
#include <iostream>
#include <chrono>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct MainScreen final : public Screen {
	PluginManager pm; // first, so it gets destroyed last

	SDL_Renderer* renderer;

	ObjectStore2 os;

	SimpleConfigModel conf;
	ContactStore4Impl cs;
	RegistryMessageModelImpl rmm;
	MessageSerializerNJ msnj;
	MessageTimeSort mts;

	SDLVideoInputService sdlvis; // sm ends the threads and closes the devices
	StreamManager sm;

	ToxClient tc;
	ToxEventLogger tel;
	ToxPrivateImpl tpi;
	AutoDirty ad;
	ToxContactModel2 tcm;
	ToxMessageManager tmm;
	ToxTransferManager ttm;
	ToxFriendFauxOfflineMessaging tffom;
#if TOMATO_TOX_AV
	ToxAVI tav;
	ToxAVVoIPModel tavvoip;
#endif

	Theme& theme;

	MediaMetaInfoLoader mmil;
	ToxAvatarManager tam;
	ToxAvatarSender tas;

	SDLRendererTextureUploader sdlrtu;
	//OpenGLTextureUploader ogltu;

	ToxAvatarLoader tal;
	ContactTextureCache contact_tc;
	MessageImageLoader mil;
	MessageTextureCache msg_tc;

	std::unique_ptr<SystemTray> st;
	StatusIndicator si;
	ChatGui4 cg;
	SettingsWindow sw;
	ObjectStoreUI osui;
	ToxUIUtils tuiu;
	ToxDHTCapHisto tdch;
	ToxNetprofUI tnui;
	StreamManagerUI smui;
	DebugVideoTap dvt;


	bool _show_tool_style_editor {false};
	bool _show_tool_metrics {false};
	bool _show_tool_debug_log {false};
	bool _show_tool_id_stack {false};
	bool _show_tool_demo {false};

	bool _window_focused {true};
	bool _window_hidden {false};
	uint64_t _window_hidden_ts {0};
	float _time_since_event {0.f};

	std::vector<std::string> _dopped_files;

	MainScreen(const SimpleConfigModel& conf_, SDL_Renderer* renderer_, Theme& theme_, std::string save_path, std::string save_password, std::string new_username, std::vector<std::string> plugins);
	~MainScreen(void);

	bool handleEvent(SDL_Event& e) override;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* render(float time_delta, bool&) override;
	Screen* tick(float time_delta, bool&) override;

	// 0 - normal
	// 1 - reduced
	// 2 - power save
	int _fps_perf_mode {0};
	// 0 - normal
	// 1 - power save
	int _compute_perf_mode {0};

	float _render_interval {1.f/60.f};
	float _min_tick_interval {0.f};

	float nextRender(void) override { return _render_interval; }
	float nextTick(void) override { return _min_tick_interval; }
};

