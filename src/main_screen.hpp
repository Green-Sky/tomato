#pragma once

#include "./screen.hpp"

#include <solanaceae/util/simple_config_model.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>
#include <solanaceae/plugin/plugin_manager.hpp>
#include <solanaceae/toxcore/tox_event_logger.hpp>

#include "./tox_client.hpp"

#include <string>
#include <iostream>
#include <chrono>

struct MainScreen final : public Screen {
	std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();

	SimpleConfigModel conf;
	Contact3Registry cr;
	RegistryMessageModel rmm;

	PluginManager pm;

	ToxEventLogger tel{std::cout};
	ToxClient tc;

	//OpenGLTextureUploader ogltu;


	MainScreen(std::string save_path);
	~MainScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

