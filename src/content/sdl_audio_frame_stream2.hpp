#pragma once

#include "./frame_stream2.hpp"
#include "./audio_stream.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <variant>
#include <vector>
#include <thread>

// we dont have to multiplex ourself, because sdl streams and virtual devices already do this, but we do it anyway
using SDLAudioInputFrameStream2Multiplexer = QueuedFrameStream2Multiplexer<AudioFrame>;
using SDLAudioInputFrameStream2 = SDLAudioInputFrameStream2Multiplexer::reader_type_t; // just use the default for now

// object components?
struct SDLAudioInputDevice : protected SDLAudioInputFrameStream2Multiplexer {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	std::atomic<bool> _thread_should_quit {false};
	std::thread _thread;

	// construct source and start thread
	// TODO: optimize so the thread is not always running
	SDLAudioInputDevice(void);

	// stops the thread and closes the device?
	~SDLAudioInputDevice(void);

	using SDLAudioInputFrameStream2Multiplexer::aquireReader;
	using SDLAudioInputFrameStream2Multiplexer::releaseReader;
};

struct SDLAudioOutputDevice {
};

