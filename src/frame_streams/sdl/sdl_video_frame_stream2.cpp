#include "./sdl_video_frame_stream2.hpp"

#include <chrono>

#include <iostream>

SDLVideo2InputDevice::SDLVideo2InputDevice(void) {
	int devcount {0};
	SDL_CameraID *devices = SDL_GetCameras(&devcount);

	if (devices == nullptr || devcount < 1) {
		throw int(2); // TODO: proper error code
	}

	// pick the last (usually the newest device)
	_dev = devices[devcount-1];

	SDL_free(devices);
}

SDLVideo2InputDevice::SDLVideo2InputDevice(const SDL_CameraID dev) : _dev(dev) {
	// nothing else?
}

SDLVideo2InputDevice::~SDLVideo2InputDevice(void) {
	if (_thread.joinable()) {
		assert(_ref == 0);
		_thread.join();
	}
}

std::shared_ptr<FrameStream2I<SDLVideoFrame>> SDLVideo2InputDevice::subscribe(void) {
	const int prev_ref = _ref++;
	if (prev_ref == 0) {
		// there was previously no stream, we assume no thread
		// open device here? or on the thread?

		SDL_CameraSpec spec {
			// FORCE a different pixel format
			SDL_PIXELFORMAT_UNKNOWN,
			//SDL_PIXELFORMAT_YUY2,
			SDL_COLORSPACE_UNKNOWN,
			//SDL_COLORSPACE_YUV_DEFAULT,

			1280, 720,

			60, 1
		};

		// choose a good spec, large res but <= 1080p
		int speccount {0};
		SDL_CameraSpec** specs = SDL_GetCameraSupportedFormats(_dev, &speccount);
		if (specs != nullptr) {
			spec = *specs[speccount-1]; // start with last, as its usually the smallest
			for (int spec_i = 1; spec_i < speccount; spec_i++) {
				if (specs[spec_i]->height > 1080) {
					continue;
				}
				if (spec.height > specs[spec_i]->height) {
					continue;
				}
				if (
					float(spec.framerate_numerator)/float(spec.framerate_denominator)
					>
					float(specs[spec_i]->framerate_numerator)/float(specs[spec_i]->framerate_denominator)
				) {
					continue;
				}

				if (spec.format == SDL_PIXELFORMAT_NV12 && specs[spec_i]->format == SDL_PIXELFORMAT_YUY2) {
					// HACK: prefer nv12 over yuy2
					continue;
				}

				// seems to be better
				spec = *specs[spec_i];
			}
			SDL_free(specs);
		}

		std::unique_ptr<SDL_Camera, decltype(&SDL_CloseCamera)> camera {nullptr, &SDL_CloseCamera};

		camera = {
			//SDL_OpenCamera(device, nullptr),
			SDL_OpenCamera(_dev, &spec),
			&SDL_CloseCamera
		};

		if (!camera) {
			std::cerr << "SDLVID error: failed opening camera device\n";
			_ref--;
			return nullptr;
		}

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

