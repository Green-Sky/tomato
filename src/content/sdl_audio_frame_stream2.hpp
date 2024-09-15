#pragma once

#include "./frame_stream2.hpp"
#include "./audio_stream.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <variant>
#include <vector>
#include <thread>

// we dont have to multicast ourself, because sdl streams and virtual devices already do this, but we do it anyway
using AudioFrameStream2MultiSource = FrameStream2MultiSource<AudioFrame>;
using AudioFrameStream2 = AudioFrameStream2MultiSource::sub_stream_type_t; // just use the default for now

// object components?

// source
struct SDLAudioInputDeviceDefault : protected AudioFrameStream2MultiSource {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	std::atomic<bool> _thread_should_quit {false};
	std::thread _thread;

	// construct source and start thread
	// TODO: optimize so the thread is not always running
	SDLAudioInputDeviceDefault(void);

	// stops the thread and closes the device?
	~SDLAudioInputDeviceDefault(void);

	using AudioFrameStream2MultiSource::subscribe;
	using AudioFrameStream2MultiSource::unsubscribe;
};

// sink
struct SDLAudioOutputDeviceDefaultInstance : public AudioFrameStream2I {
	std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> _stream;

	uint32_t _last_sample_rate {48'000};
	size_t _last_channels {0};
	SDL_AudioFormat _last_format {SDL_AUDIO_S16};

	SDLAudioOutputDeviceDefaultInstance(void);
	SDLAudioOutputDeviceDefaultInstance(SDLAudioOutputDeviceDefaultInstance&& other);

	~SDLAudioOutputDeviceDefaultInstance(void);

	int32_t size(void) override;
	std::optional<AudioFrame> pop(void) override;
	bool push(const AudioFrame& value) override;
};

// constructs entirely new streams, since sdl handles sync and mixing for us (or should)
struct SDLAudioOutputDeviceDefaultSink : public FrameStream2SinkI<AudioFrame> {
	// TODO: pause device?

	~SDLAudioOutputDeviceDefaultSink(void);

	std::shared_ptr<FrameStream2I<AudioFrame>> subscribe(void) override;
	bool unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame>>& sub) override;
};

