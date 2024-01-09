#pragma once

#include "./screen.hpp"

#include <solanaceae/util/simple_config_model.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/message3/message_time_sort.hpp>
#include <solanaceae/plugin/plugin_manager.hpp>
#include <solanaceae/toxcore/tox_event_logger.hpp>
#include "./tox_private_impl.hpp"

#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include <solanaceae/tox_messages/tox_message_manager.hpp>
#include <solanaceae/tox_messages/tox_transfer_manager.hpp>

#include "./tox_client.hpp"
#include "./auto_dirty.hpp"

#include "./media_meta_info_loader.hpp"
#include "./tox_avatar_manager.hpp"

#include "./sdlrenderer_texture_uploader.hpp"
#include "./chat_gui4.hpp"
#include "./settings_window.hpp"
#include "./tox_ui_utils.hpp"
#include "./tox_dht_cap_histo.hpp"

#include <string>
#include <iostream>
#include <chrono>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct MainScreen final : public Screen {
	SDL_Renderer* renderer;

	SimpleConfigModel conf;
	Contact3Registry cr;
	RegistryMessageModel rmm;
	MessageTimeSort mts;

	PluginManager pm;

	ToxEventLogger tel{std::cout};
	ToxClient tc;
	ToxPrivateImpl tpi;
	AutoDirty ad;
	ToxContactModel2 tcm;
	ToxMessageManager tmm;
	ToxTransferManager ttm;

	MediaMetaInfoLoader mmil;
	ToxAvatarManager tam;

	SDLRendererTextureUploader sdlrtu;
	//OpenGLTextureUploader ogltu;

	ChatGui4 cg;
	SettingsWindow sw;
	ToxUIUtils tuiu;
	ToxDHTCapHisto tdch;

	bool _window_hidden {false};
	bool _window_hidden_ts {0};
	float _time_since_event {0.f};

	MainScreen(SDL_Renderer* renderer_, std::string save_path, std::string save_password, std::vector<std::string> plugins);
	~MainScreen(void);

	bool handleEvent(SDL_Event& e) override;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* render(float time_delta, bool&) override;
	Screen* tick(float time_delta, bool&) override;

	// 0 - normal
	// 1 - reduced
	// 2 - power save
	int _fps_perf_mode {1};
	// 0 - normal
	// 1 - power save
	int _compute_perf_mode {0};

	float _render_interval {1.f/60.f};
	float _min_tick_interval {0.f};

	float nextRender(void) override { return _render_interval; }
	float nextTick(void) override { return _min_tick_interval; }
};

