#include "./sdl_video_frame_stream2.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>

SDLVideoCameraContent::SDLVideoCameraContent(void) {
	int devcount {0};
	SDL_CameraDeviceID *devices = SDL_GetCameraDevices(&devcount);
	std::cout << "SDL Camera Driver: " << SDL_GetCurrentCameraDriver() << "\n";

	if (devices == nullptr || devcount < 1) {
		throw int(1); // TODO: proper exception?
	}

	std::cout << "### found cameras:\n";
	for (int i = 0; i < devcount; i++) {
		const SDL_CameraDeviceID device = devices[i];

		char *name = SDL_GetCameraDeviceName(device);
		std::cout << "  - Camera #" << i << ": " << name << "\n";
		SDL_free(name);

		int speccount {0};
		SDL_CameraSpec* specs = SDL_GetCameraDeviceSupportedFormats(device, &speccount);
		if (specs == nullptr) {
			std::cout << "    - no supported spec\n";
		} else {
			for (int spec_i = 0; spec_i < speccount; spec_i++) {
				std::cout << "    - " << specs[spec_i].width << "x" << specs[spec_i].height << "@" << float(specs[spec_i].interval_denominator)/specs[spec_i].interval_numerator << " " << SDL_GetPixelFormatName(specs[spec_i].format) << "\n";

			}

			SDL_free(specs);
		}
	}

	{
		SDL_CameraSpec spec {
			// FORCE a diffrent pixel format
			SDL_PIXELFORMAT_RGBA8888,

			//1280, 720,
			//640, 360,
			640, 480,

			1, 30
		};
		_camera = {
			SDL_OpenCameraDevice(devices[0], &spec),
			&SDL_CloseCamera
		};
	}
	SDL_free(devices);
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
		auto* format_name = SDL_GetPixelFormatName(spec.format);
		std::cout << "camera format: " << format_name << "\n";
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

#if 0
			{ // test copy to trigger bug
				SDL_Surface* test_surf = SDL_CreateSurface(
					sdl_frame_next->w,
					sdl_frame_next->h,
					SDL_PIXELFORMAT_RGBA8888
				);

				assert(test_surf != nullptr);

				SDL_BlitSurface(sdl_frame_next, nullptr, test_surf, nullptr);

				SDL_DestroySurface(test_surf);
			}
#endif

			bool someone_listening {false};
			{
				SDLVideoFrame new_frame_non_owning {
					timestampNS,
					sdl_frame_next
				};

				// creates surface copies
				someone_listening = push(new_frame_non_owning);
			}
			SDL_ReleaseCameraFrame(_camera.get(), sdl_frame_next);

			if (someone_listening) {
				// double the interval on acquire
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(interval*1000*0.5)));
			} else {
				std::cerr << "i guess no one is listening\n";
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

	// HACK: sdl is buggy and freezes otherwise. it is likely still possible, but rare to freeze here
	// flush unused frames
#if 1
	while (true) {
		SDL_Surface* sdl_frame_next = SDL_AcquireCameraFrame(_camera.get(), nullptr);
		if (sdl_frame_next != nullptr) {
			SDL_ReleaseCameraFrame(_camera.get(), sdl_frame_next);
		} else {
			break;
		}
	}
#endif
}
