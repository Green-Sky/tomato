#include "./sdl_video_frame_stream2.hpp"

#include <chrono>
#include <algorithm>

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
		if (specs != nullptr && speccount > 0) {
			spec = *specs[speccount-1]; // start with last, as its usually the smallest
			for (int spec_i = 0; spec_i < speccount; spec_i++) {
				if (specs[spec_i]->height > 1080) {
					continue;
				}

				if (spec.format == SDL_PIXELFORMAT_MJPG && spec.format != specs[spec_i]->format) {
					// we hard prefer anything else over mjpg
					spec = *specs[spec_i];
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

				// HACK: prefer nv12 over yuy2, SDL has sse2 optimized conversion for our usecase
				if (spec.format == SDL_PIXELFORMAT_NV12 && specs[spec_i]->format == SDL_PIXELFORMAT_YUY2) {
					continue;
				}

				// seems to be better
				spec = *specs[spec_i];
			}

		}
		if (specs != nullptr) {
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

		_thread = std::thread([this, camera = std::move(camera)](void) {
			bool use_chrono_fallback = false;
			Uint64 last_timestampUS = 0;

			double intervalUS_avg = 1.;

			while (_ref > 0) {
				Uint64 timestampNS = 0;

				// aquire frame
				SDL_Surface* sdl_frame_next = SDL_AcquireCameraFrame(camera.get(), &timestampNS);

				if (sdl_frame_next != nullptr) {
					Uint64 timestampUS_correct = timestampNS/1000;
					if (!use_chrono_fallback) {
						if (timestampNS == 0 || last_timestampUS == timestampUS_correct) {
							use_chrono_fallback = true;
							std::cerr << "SDLVID: invalid or unreliable timestampNS from sdl, falling back to own mesurements!\n";
							timestampUS_correct = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
						} else if (last_timestampUS == 0) {
							last_timestampUS = timestampUS_correct;
							// HACK: skip first frame
							std::cerr << "SDLVID: skipping first frame\n";
							SDL_ReleaseCameraFrame(camera.get(), sdl_frame_next);
							continue;
						}
					} else {
						timestampUS_correct = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
					}

					if (last_timestampUS != 0 && timestampUS_correct != 0 && last_timestampUS != timestampUS_correct && last_timestampUS < timestampUS_correct) {
						const double r = 0.15;
						intervalUS_avg = std::clamp(intervalUS_avg * (1.-r) + (timestampUS_correct-last_timestampUS) * r, 1000., 500.*1000.);
					}

					SDLVideoFrame new_frame_non_owning {
						timestampUS_correct,
						sdl_frame_next
					};

					// creates surface copies
					push(new_frame_non_owning);


					last_timestampUS = timestampUS_correct;

					SDL_ReleaseCameraFrame(camera.get(), sdl_frame_next);
				}

				// sleep for interval
				// TODO: do we really need half?
				std::this_thread::sleep_for(std::chrono::milliseconds(int64_t((intervalUS_avg/1000)*0.5)));
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

