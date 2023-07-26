#pragma once

#include "./screen.hpp"

#include <solanaceae/util/simple_config_model.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/plugin/plugin_manager.hpp>
#include <solanaceae/toxcore/tox_event_logger.hpp>

#include <solanaceae/tox_contacts/tox_contact_model2.hpp>
#include <solanaceae/tox_messages/tox_message_manager.hpp>
#include <solanaceae/tox_messages/tox_transfer_manager.hpp>

#include "./tox_client.hpp"

#include "./sdlrenderer_texture_uploader.hpp"

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

	PluginManager pm;

	ToxEventLogger tel{std::cout};
	ToxClient tc;
	ToxContactModel2 tcm;
	ToxMessageManager tmm;
	ToxTransferManager ttm;

	SDLRendererTextureUploader sdlrtu;
	//OpenGLTextureUploader ogltu;


	MainScreen(SDL_Renderer* renderer_, std::string save_path);
	~MainScreen(void);

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

