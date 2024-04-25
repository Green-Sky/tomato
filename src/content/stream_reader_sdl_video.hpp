#pragma once

#include "./stream_reader.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <memory>

struct SDLVideoFrameStreamReader : public RawFrameStreamReaderI {
	// meh, empty default
	std::unique_ptr<SDL_Camera, decltype(&SDL_CloseCamera)> _camera;
	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> _surface;

	SDLVideoFrameStreamReader(void);

	struct VideoFrame {
		int32_t width {0};
		int32_t height {0};

		uint64_t timestampNS {0};

		ByteSpan data;
	};

	// data owned by StreamReader, overwritten by next call to getNext*()
	VideoFrame getNextVideoFrameRGBA(void);

	public: // interface
		int64_t have(void) override;
		ByteSpan getNext(void) override;
};

