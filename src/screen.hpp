#pragma once

#include <SDL3/SDL.h>

// all time values are in seconds
struct Screen {
	virtual ~Screen(void) = default;

	// return true if handled
	virtual bool handleEvent(SDL_Event&) { return false; }

	// return nullptr if not next
	// sets bool quit to true if exit
	// both render and tick get called in the selfreported intervals
	virtual Screen* render(float time_delta, bool& quit) = 0; // pure since this is a graphical app
	virtual Screen* tick(float time_delta, bool& quit) = 0;

	// TODO: const?
	virtual float nextRender(void) { return 1.f/60.f; }
	virtual float nextTick(void) { return 0.03f; }
};

