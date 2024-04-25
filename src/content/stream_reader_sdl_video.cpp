#include "./stream_reader_sdl_video.hpp"

#include <iostream>

SDLVideoFrameStreamReader::SDLVideoFrameStreamReader() : _camera{nullptr, &SDL_CloseCamera}, _surface{nullptr, &SDL_DestroySurface} {
	// enumerate
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
	if (SDL_GetCameraFormat(_camera.get(), &spec) < 0) {
		// meh
	} else {
		// interval
		float interval = float(spec.interval_numerator)/float(spec.interval_denominator);
		std::cout << "camera interval: " << interval*1000 << "ms\n";
	}
}

SDLVideoFrameStreamReader::VideoFrame SDLVideoFrameStreamReader::getNextVideoFrameRGBA(void) {
	if (!static_cast<bool>(_camera)) {
		return {};
	}

	Uint64 timestampNS = 0;
	SDL_Surface* frame_next = SDL_AcquireCameraFrame(_camera.get(), &timestampNS);

	// no new frame yet, or error
	if (frame_next == nullptr) {
		//std::cout << "failed acquiring frame\n";
		return {};
	}

	// TODO: investigate zero copy
	_surface = {
		SDL_ConvertSurfaceFormat(frame_next, SDL_PIXELFORMAT_RGBA8888),
		&SDL_DestroySurface
	};

	SDL_ReleaseCameraFrame(_camera.get(), frame_next);

	SDL_LockSurface(_surface.get());

	return {
		_surface->w,
		_surface->h,
		timestampNS,
		{
			reinterpret_cast<const uint8_t*>(_surface->pixels),
			uint64_t(_surface->w*_surface->h*4) // rgba
		}
	};
}

int64_t SDLVideoFrameStreamReader::have(void) {
	return -1;
}

ByteSpan SDLVideoFrameStreamReader::getNext(void) {
	return {};
}

