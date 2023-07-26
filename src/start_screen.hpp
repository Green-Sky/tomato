#pragma once

#include "./screen.hpp"

// fwd
extern "C" {
	struct SDL_Renderer;
} // C

struct StartScreen final : public Screen {
	SDL_Renderer* _renderer;

	StartScreen(void) = delete;
	StartScreen(SDL_Renderer* renderer);
	~StartScreen(void) = default;

	// return nullptr if not next
	// sets bool quit to true if exit
	Screen* poll(bool&) override;
};

