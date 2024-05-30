#pragma once

#include "./screen.hpp"

#include "./chat_gui/theme.hpp"
#include "./chat_gui/file_selector.hpp"

#include <solanaceae/util/simple_config_model.hpp>

#include <vector>
#include <string>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct StartScreen final : public Screen {
	SDL_Renderer* _renderer;
	Theme& _theme;
	SimpleConfigModel _conf;
	FileSelector _fss;

	bool _new_save {false};
	std::string _user_name {"unnamed-tomato"};

	bool _show_password {false};
	std::string _password;

	std::string _tox_profile_path {"unnamed-tomato.tox"};
	std::vector<std::string> queued_plugin_paths;

	StartScreen(void) = delete;
	StartScreen(const std::vector<std::string_view>& args, SDL_Renderer* renderer, Theme& theme);
	~StartScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* render(float, bool&) override;
	Screen* tick(float, bool&) override;

	// use default nextRender and nextTick
};

