#pragma once

#include "./video.hpp"
#include "../frame_stream2.hpp"
#include "../multi_source.hpp"

#include <atomic>
#include <thread>

// tips: you can force a SDL vido driver by setting an env:
// SDL_CAMERA_DRIVER=v4l2
// SDL_CAMERA_DRIVER=pipewire
// etc.

// while a stream is subscribed, have the camera device open
// and aquire and push frames from a thread
struct SDLVideo2InputDevice : public FrameStream2MultiSource<SDLVideoFrame> {
	std::atomic_uint _ref {0};
	std::thread _thread;

	// TODO: device id
	SDLVideo2InputDevice(void);
	virtual ~SDLVideo2InputDevice(void);

	// we hook int multi source
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override;
	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override;
};

