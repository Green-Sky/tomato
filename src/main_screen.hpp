#pragma once

#include "./screen.hpp"

#include <solanaceae/util/simple_config_model.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/message3/message_time_sort.hpp>
#include <solanaceae/plugin/plugin_manager.hpp>
#include <solanaceae/toxcore/tox_event_logger.hpp>

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

#include <string>
#include <iostream>
#include <chrono>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct MainScreen final : public Screen {
	SDL_Renderer* renderer;

	std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();

	SimpleConfigModel conf;
	Contact3Registry cr;
	RegistryMessageModel rmm;
	MessageTimeSort mts;

	PluginManager pm;

	ToxEventLogger tel{std::cout};
	ToxClient tc;
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

	MainScreen(SDL_Renderer* renderer_, std::string save_path, std::string save_password, std::vector<std::string> plugins);
	~MainScreen(void);

	bool handleEvent(SDL_Event& e) override;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

