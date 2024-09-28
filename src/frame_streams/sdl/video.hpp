#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>

// https://youtu.be/71Iw4Q74OaE

inline void nopSurfaceDestructor(SDL_Surface*) {}

// this is very sdl specific
// but allows us to autoconvert between formats (to a degree)
struct SDLVideoFrame {
	// micro seconds (nano is way too much)
	uint64_t timestampUS {0};

	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface {nullptr, &SDL_DestroySurface};

	// special non-owning constructor
	SDLVideoFrame(
		uint64_t ts,
		SDL_Surface* surf
	) {
		timestampUS = ts;
		surface = {surf, &nopSurfaceDestructor};
	}
	SDLVideoFrame(SDLVideoFrame&& other) = default;
	// copy
	SDLVideoFrame(const SDLVideoFrame& other) {
		timestampUS = other.timestampUS;
		if (static_cast<bool>(other.surface)) {
			surface = {
				SDL_DuplicateSurface(other.surface.get()),
				&SDL_DestroySurface
			};
		}
	}
	SDLVideoFrame& operator=(const SDLVideoFrame& other) = delete;
};

