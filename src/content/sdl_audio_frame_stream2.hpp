#pragma once

#include "./frame_stream2.hpp"
#include "./audio_stream.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

// we dont have to multicast ourself, because sdl streams and virtual devices already do this

// source
// opens device
// creates a sdl audio stream for each subscribed reader stream
struct SDLAudioInputDevice : public FrameStream2SourceI<AudioFrame> {
	// held by instances
	using sdl_stream_type = std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)>;

	SDL_AudioDeviceID _configured_device_id {0};
	SDL_AudioDeviceID _virtual_device_id {0};

	std::vector<std::shared_ptr<FrameStream2I<AudioFrame>>> _streams;

	SDLAudioInputDevice(void);
	SDLAudioInputDevice(SDL_AudioDeviceID conf_device_id);
	~SDLAudioInputDevice(void);

	std::shared_ptr<FrameStream2I<AudioFrame>> subscribe(void) override;
	bool unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame>>& sub) override;
};

// sink
// constructs entirely new streams, since sdl handles sync and mixing for us (or should)
struct SDLAudioOutputDeviceDefaultSink : public FrameStream2SinkI<AudioFrame> {
	// TODO: pause device?

	~SDLAudioOutputDeviceDefaultSink(void);

	std::shared_ptr<FrameStream2I<AudioFrame>> subscribe(void) override;
	bool unsubscribe(const std::shared_ptr<FrameStream2I<AudioFrame>>& sub) override;
};

