#include "./sdl_video_frame_stream2.hpp"

#include <chrono>

#include <iostream>

SDLVideo2InputDevice::SDLVideo2InputDevice(void) {
	int devcount {0};
	SDL_CameraID *devices = SDL_GetCameras(&devcount);
	std::cout << "SDLVID: SDL Camera Driver: " << SDL_GetCurrentCameraDriver() << "\n";

	if (devices == nullptr || devcount < 1) {
		throw int(2); // TODO: proper error code
	}

	std::cout << "SDLVID: found cameras:\n";
	for (int i = 0; i < devcount; i++) {
		const SDL_CameraID device = devices[i];

		const char *name = SDL_GetCameraName(device);
		std::cout << "  - Camera #" << i << ": " << name << "\n";

		int speccount {0};
		SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(device, &speccount);
		if (specs == nullptr) {
			std::cout << "    - no supported spec\n";
		} else {
			for (int spec_i = 0; spec_i < speccount; spec_i++) {
				std::cout << "    - " << specs[spec_i]->width << "x" << specs[spec_i]->height << "@" << float(specs[spec_i]->framerate_numerator)/specs[spec_i]->framerate_denominator << "fps " << SDL_GetPixelFormatName(specs[spec_i]->format) << "\n";

			}
			SDL_free(specs);
		}
	}
	SDL_free(devices);
}

SDLVideo2InputDevice::~SDLVideo2InputDevice(void) {
}

std::shared_ptr<FrameStream2I<SDLVideoFrame>> SDLVideo2InputDevice::subscribe(void) {
	const int prev_ref = _ref++;
	if (prev_ref == 0) {
		// there was previously no stream, we assume no thread
		// open device here? or on the thread?

		int devcount {0};
		SDL_CameraID *devices = SDL_GetCameras(&devcount);
		//std::cout << "SDL Camera Driver: " << SDL_GetCurrentCameraDriver() << "\n";

		if (devices == nullptr || devcount < 1) {
			_ref--;
			// error/no devices, should we do this in the constructor?
			SDL_free(devices);
			return nullptr;
		}

		// TODO: relist on device open?
		//std::cout << "### found cameras:\n";
		//for (int i = 0; i < devcount; i++) {
		//    const SDL_CameraID device = devices[i];

		//    const char *name = SDL_GetCameraName(device);
		//    std::cout << "  - Camera #" << i << ": " << name << "\n";

		//    int speccount {0};
		//    SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(device, &speccount);
		//    if (specs == nullptr) {
		//        std::cout << "    - no supported spec\n";
		//    } else {
		//        for (int spec_i = 0; spec_i < speccount; spec_i++) {
		//            std::cout << "    - " << specs[spec_i]->width << "x" << specs[spec_i]->height << "@" << float(specs[spec_i]->framerate_numerator)/specs[spec_i]->framerate_denominator << "fps " << SDL_GetPixelFormatName(specs[spec_i]->format) << "\n";

		//        }
		//        SDL_free(specs);
		//    }
		//}

		std::unique_ptr<SDL_Camera, decltype(&SDL_CloseCamera)> camera {nullptr, &SDL_CloseCamera};

		SDL_CameraSpec spec {
			// FORCE a different pixel format
			//SDL_PIXELFORMAT_UNKNOWN,
			SDL_PIXELFORMAT_YUY2,
			//SDL_COLORSPACE_UNKNOWN,
			SDL_COLORSPACE_YUV_DEFAULT,

			1280, 720,

			30, 1
		};
		camera = {
			//SDL_OpenCamera(devices[0], &spec),
			SDL_OpenCamera(devices[0], nullptr),
			&SDL_CloseCamera
		};
		SDL_free(devices);

		// seems like we need this before get format() ?
		// TODO: best would be waiting in thread, but that obv does not work well
		// TODO: sometimes if the device is/was in use it will stay 0 for ever
		while (SDL_GetCameraPermissionState(camera.get()) == 0) {
			//std::cerr << "permission for camera not granted\n";
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (SDL_GetCameraPermissionState(camera.get()) <= 0) {
			std::cerr << "SDLVID error: user denied camera permission\n";
			_ref--;
			return nullptr;
		}

		float fps {1.f};
		if (!SDL_GetCameraFormat(camera.get(), &spec)) {
			// meh
			_ref--;
			return nullptr;
		} else {
			fps = float(spec.framerate_numerator)/float(spec.framerate_denominator);
			std::cout << "SDLVID: camera fps: " << fps << "fps (" << spec.framerate_numerator << "/" << spec.framerate_denominator << ")\n";
			auto* format_name = SDL_GetPixelFormatName(spec.format);
			std::cout << "SDLVID: camera format: " << format_name << "\n";
		}

		_thread = std::thread([this, camera = std::move(camera), fps](void) {
			while (_ref > 0) {
				Uint64 timestampNS = 0;

				// aquire frame
				SDL_Surface* sdl_frame_next = SDL_AcquireCameraFrame(camera.get(), &timestampNS);

				if (sdl_frame_next != nullptr) {
					SDLVideoFrame new_frame_non_owning {
						timestampNS/1000,
						sdl_frame_next
					};

					// creates surface copies
					push(new_frame_non_owning);

					SDL_ReleaseCameraFrame(camera.get(), sdl_frame_next);
				}

				// sleep for interval
				// TODO: do we really need half?
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t((1000/fps)*0.5)));
			}
			// camera destructor closes device here
		});
		std::cout << "SDLVID: started new cam thread\n";
	}

	return FrameStream2MultiSource<SDLVideoFrame>::subscribe();
}

bool SDLVideo2InputDevice::unsubscribe(const std::shared_ptr<FrameStream2I<SDLVideoFrame>>& sub) {
	if (FrameStream2MultiSource<SDLVideoFrame>::unsubscribe(sub)) {
		if (--_ref == 0) {
			// was last stream, close device and thread
			_thread.join(); // TODO: defer to destructor or new thread?
							// this might take a moment and lock up the main thread
			std::cout << "SDLVID: ended cam thread\n";
		}
		return true;
	}
	return false;
}

