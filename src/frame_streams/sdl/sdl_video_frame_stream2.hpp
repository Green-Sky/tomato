#pragma once

#include "./video.hpp"
#include "../frame_stream2.hpp"
#include "../multi_source.hpp"

#include <atomic>
#include <thread>

// tips: you can force a SDL video driver by setting an env:
// SDL_CAMERA_DRIVER=v4l2
// SDL_CAMERA_DRIVER=pipewire
// etc.

// while a stream is subscribed, have the camera device open
// and aquire and push frames from a thread
struct SDLVideo2InputDevice : public FrameStream2MultiSource<SDLVideoFrame> {
	SDL_CameraID _dev {0};

	std::atomic_uint _ref {0};
	std::thread _thread;

	SDLVideo2InputDevice(void);
	SDLVideo2InputDevice(const SDL_CameraID dev);
	virtual ~SDLVideo2InputDevice(void);

	// we hook into multi source
	std::shared_ptr<FrameStream2I<SDLVideoFrame>> subscribe(void) override;
	bool unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) override;
};

