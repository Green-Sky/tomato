#pragma once

#include "./screen.hpp"

#include "./file_selector.hpp"

#include <vector>
#include <string>

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct StartScreen final : public Screen {
	SDL_Renderer* _renderer;
	FileSelector _fss;

	std::string tox_profile_path {"tomato.tox"};
	std::vector<std::string> queued_plugin_paths;

	StartScreen(void) = delete;
	StartScreen(SDL_Renderer* renderer);
	~StartScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

