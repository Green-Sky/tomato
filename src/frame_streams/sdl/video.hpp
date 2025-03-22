#pragma once

#include "../frame_stream2.hpp"

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

template<>
constexpr bool frameHasTimestamp<SDLVideoFrame>(void) {
	return true;
}

template<>
constexpr uint64_t frameGetTimestampDivision<SDLVideoFrame>(void) {
	// microseconds
	// technically sdl would provide them in nanoseconds... but we cut them down
	return 1'000*1'000;
}

template<>
inline uint64_t frameGetTimestamp(const SDLVideoFrame& frame) {
	return frame.timestampUS;
}

template<>
constexpr bool frameHasBytes<SDLVideoFrame>(void) {
	return true;
}

// TODO: test how performant this call is
template<>
inline uint64_t frameGetBytes(const SDLVideoFrame& frame) {
	if (!frame.surface) {
		return 0;
	}

	const auto* surf = frame.surface.get();

	if (surf->format == SDL_PIXELFORMAT_MJPG) {
		// mjpg is special, since it is compressed
		return surf->pitch;
	}

	const auto* details = SDL_GetPixelFormatDetails(surf->format);
	if (details == nullptr) {
		return 0;
	}

	return details->bytes_per_pixel * surf->w * surf->h;
}

