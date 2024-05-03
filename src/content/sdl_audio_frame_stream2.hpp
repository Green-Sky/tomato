#pragma once

#include "./frame_stream2.hpp"
#include "./audio_stream.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <variant>
#include <vector>
#include <thread>

// we dont have to multicast ourself, because sdl streams and virtual devices already do this, but we do it anyway
using AudioFrameStream2MultiStream = FrameStream2MultiStream<AudioFrame>;
using AudioFrameStream2 = AudioFrameStream2MultiStream::sub_stream_type_t; // just use the default for now

// object components?

// source
struct SDLAudioInputDeviceDefault : protected AudioFrameStream2MultiStream {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	std::atomic<bool> _thread_should_quit {false};
	std::thread _thread;

	// construct source and start thread
	// TODO: optimize so the thread is not always running
	SDLAudioInputDeviceDefault(void);

	// stops the thread and closes the device?
	~SDLAudioInputDeviceDefault(void);

	using AudioFrameStream2MultiStream::aquireSubStream;
	using AudioFrameStream2MultiStream::releaseSubStream;
};

// sink
struct SDLAudioOutputDeviceDefaultInstance : protected AudioFrameStream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _last_sample_rate {48'000};
	size_t _last_channels {0};
	SDL_AudioFormat _last_format {0};

	SDLAudioOutputDeviceDefaultInstance(void);
	SDLAudioOutputDeviceDefaultInstance(SDLAudioOutputDeviceDefaultInstance&& other);

	~SDLAudioOutputDeviceDefaultInstance(void);

	int32_t size(void) override;
	std::optional<AudioFrame> pop(void) override;
	bool push(const AudioFrame& value) override;
};

// constructs entirely new streams, since sdl handles sync and mixing for us (or should)
struct SDLAudioOutputDeviceDefaultFactory {
	// TODO: pause device?

	SDLAudioOutputDeviceDefaultInstance create(void);
};

