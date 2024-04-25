#include "./sdl_video_frame_stream2.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

SDLVideoCameraContent::SDLVideoCameraContent(void) {
	int devcount = 0;
	SDL_CameraDeviceID *devices = SDL_GetCameraDevices(&devcount);

	if (devices == nullptr || devcount < 1) {
		throw int(1); // TODO: proper exception?
	}

	std::cout << "### found cameras:\n";
	for (int i = 0; i < devcount; i++) {
		const SDL_CameraDeviceID device = devices[i];
		char *name = SDL_GetCameraDeviceName(device);

		std::cout << "  - Camera #" << i << ": " << name << "\n";

		SDL_free(name);
	}

	_camera = {
		SDL_OpenCameraDevice(devices[0], nullptr),
		&SDL_CloseCamera
	};
	if (!static_cast<bool>(_camera)) {
		throw int(2);
	}

	SDL_CameraSpec spec;
	float interval {0.1f};
	if (SDL_GetCameraFormat(_camera.get(), &spec) < 0) {
		// meh
	} else {
		// interval
		interval = float(spec.interval_numerator)/float(spec.interval_denominator);
		std::cout << "camera interval: " << interval*1000 << "ms\n";
	}

	_thread = std::thread([this, interval](void) {
		while (!_thread_should_quit) {
			Uint64 timestampNS = 0;
			SDL_Surface* sdl_frame_next = SDL_AcquireCameraFrame(_camera.get(), &timestampNS);

			// no new frame yet, or error
			if (sdl_frame_next == nullptr) {
				// only sleep 1/10, we expected a frame
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval*1000 / 10)));
				continue;
			}

			SDLVideoFrame new_frame {
				timestampNS,
				sdl_frame_next
			};

			bool someone_listening = pushValue(new_frame);

			SDL_ReleaseCameraFrame(_camera.get(), sdl_frame_next);

			if (someone_listening) {
				// TODO: maybe double the freq?
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval*1000)));
			} else {
				// we just sleep 4x as long, bc no one is listening
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval*1000*4)));
			}
		}
	});
}

SDLVideoCameraContent::~SDLVideoCameraContent(void) {
	_thread_should_quit = true;
	_thread.join();
	// TODO: what to do if readers are still present?
}
