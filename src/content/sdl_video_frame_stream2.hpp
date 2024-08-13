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
			// TODO: use SDL_DuplicateSurface()
			surface = {
				SDL_CreateSurface(
					other.surface->w,
					other.surface->h,
					other.surface->format
				),
				&SDL_DestroySurface
			};
			SDL_BlitSurface(other.surface.get(), nullptr, surface.get(), nullptr);
		}
	}
	SDLVideoFrame& operator=(const SDLVideoFrame& other) = delete;
};

using SDLVideoFrameStream2MultiStream = FrameStream2MultiStream<SDLVideoFrame>;
using SDLVideoFrameStream2 = SDLVideoFrameStream2MultiStream::sub_stream_type_t; // just use the default for now

struct SDLVideoCameraContent : protected SDLVideoFrameStream2MultiStream {
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
	using SDLVideoFrameStream2MultiStream::aquireSubStream;
	using SDLVideoFrameStream2MultiStream::releaseSubStream;
};

