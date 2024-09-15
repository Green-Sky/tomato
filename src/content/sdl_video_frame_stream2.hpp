#pragma once

#include "./frame_stream2.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <thread>

inline void nopSurfaceDestructor(SDL_Surface*) {}

// this is very sdl specific
struct SDLVideoFrame {
	// TODO: sequence numbering?
	uint64_t timestampNS {0};

	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface {nullptr, &SDL_DestroySurface};

	// special non-owning constructor?
	SDLVideoFrame(
		uint64_t ts,
		SDL_Surface* surf
	) {
		timestampNS = ts;
		surface = {surf, &nopSurfaceDestructor};
	}
	// copy
	SDLVideoFrame(const SDLVideoFrame& other) {
		timestampNS = other.timestampNS;
		if (static_cast<bool>(other.surface)) {
			//surface = {
			//    SDL_CreateSurface(
			//        other.surface->w,
			//        other.surface->h,
			//        other.surface->format
			//    ),
			//    &SDL_DestroySurface
			//};
			//SDL_BlitSurface(other.surface.get(), nullptr, surface.get(), nullptr);
			surface = {
				SDL_DuplicateSurface(other.surface.get()),
			    &SDL_DestroySurface
			};
		}
	}
	SDLVideoFrame& operator=(const SDLVideoFrame& other) = delete;
};

using SDLVideoFrameStream2MultiSource = FrameStream2MultiSource<SDLVideoFrame>;
using SDLVideoFrameStream2 = SDLVideoFrameStream2MultiSource::sub_stream_type_t; // just use the default for now

struct SDLVideoCameraContent : public SDLVideoFrameStream2MultiSource {
	// meh, empty default
	std::unique_ptr<SDL_Camera, decltype(&SDL_CloseCamera)> _camera {nullptr, &SDL_CloseCamera};
	std::atomic<bool> _thread_should_quit {false};
	std::thread _thread;

	// construct source and start thread
	// TODO: optimize so the thread is not always running
	SDLVideoCameraContent(void);

	// stops the thread and closes the camera
	~SDLVideoCameraContent(void);

	// make only some of writer public
	using SDLVideoFrameStream2MultiSource::subscribe;
	using SDLVideoFrameStream2MultiSource::unsubscribe;
};

