#pragma once

#include "./stream_reader.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <memory>

struct SDLAudioFrameStreamReader : public RawFrameStreamReaderI {
	// count samples per buffer
	const int32_t _buffer_size {1024};
	std::vector<int16_t> _buffer;
	size_t _remaining_size {0}; // data still in buffer, that was remaining from last call and not enough to fill a full frame

	// meh, empty default
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	// buffer_size in number of samples
	SDLAudioFrameStreamReader(int32_t buffer_size = 1024);

	// data owned by StreamReader, overwritten by next call to getNext*()
	Span<int16_t> getNextAudioFrame(void);

	public: // interface
		int64_t have(void) override;

		ByteSpan getNext(void) override;
};

