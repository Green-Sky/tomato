#pragma once

#include "./screen.hpp"

#include "./chat_gui/file_selector.hpp"

#include <vector>
#include <string>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct StartScreen final : public Screen {
	SDL_Renderer* _renderer;
	FileSelector _fss;

	bool _new_save {false};

	bool _show_password {false};
	std::string _password;

	std::string tox_profile_path {"tomato.tox"};
	std::vector<std::string> queued_plugin_paths;

	StartScreen(void) = delete;
	StartScreen(SDL_Renderer* renderer);
	~StartScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* render(float, bool&) override;
	Screen* tick(float, bool&) override;

	// use default nextRender and nextTick
};

