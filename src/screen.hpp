#pragma once

#include <SDL3/SDL.h>

struct Screen {
	virtual ~Screen(void) = default;

	// return true if handled
	virtual bool handleEvent(SDL_Event& e) { return false; }

	// return nullptr if not next
	// sets bool quit to true if exit
	virtual Screen* poll(bool& quit) = 0;
};

