#pragma once

#include "./video.hpp"
#include "../frame_stream2.hpp"

#include <cassert>

#include <iostream> // meh

static bool isFormatYUV(SDL_PixelFormat f) {
	return
		f == SDL_PIXELFORMAT_YV12 ||
		f == SDL_PIXELFORMAT_IYUV ||
		f == SDL_PIXELFORMAT_YUY2 ||
		f == SDL_PIXELFORMAT_UYVY ||
		f == SDL_PIXELFORMAT_YVYU ||
		f == SDL_PIXELFORMAT_NV12 ||
		f == SDL_PIXELFORMAT_NV21 ||
		f == SDL_PIXELFORMAT_P010
	;
}

SDL_Surface* convertYUY2_IYUV(SDL_Surface* surf);

template<typename RealStream>
struct PushConversionVideoStream : public RealStream {
	SDL_PixelFormat _forced_format {SDL_PIXELFORMAT_IYUV};

	template<typename... Args>
	PushConversionVideoStream(SDL_PixelFormat forced_format, Args&&... args) : RealStream(std::forward<Args>(args)...), _forced_format(forced_format) {}
	~PushConversionVideoStream(void) {}

	bool push(const SDLVideoFrame& value) override {
		SDL_Surface* surf = value.surface.get();
		if (surf->format != _forced_format) {
			//std::cerr << "DTC: need to convert from " << SDL_GetPixelFormatName(converted_surf->format) << " to SDL_PIXELFORMAT_IYUV\n";
			if (surf->format == SDL_PIXELFORMAT_YUY2 && _forced_format == SDL_PIXELFORMAT_IYUV) {
				// optimized custom impl

				//auto start = Message::getTimeMS();
				surf = convertYUY2_IYUV(surf);
				//auto end = Message::getTimeMS();
				// 3ms
				//std::cerr << "DTC: timing " << SDL_GetPixelFormatName(converted_surf->format) << "->SDL_PIXELFORMAT_IYUV: " << end-start << "ms\n";
			} else if (isFormatYUV(surf->format)) {
				// TODO: fix sdl rgb->yuv conversion resulting in too dark (colorspace) issues
				// https://github.com/libsdl-org/SDL/issues/10877

				// meh, need to convert to rgb as a stopgap

				//auto start = Message::getTimeMS();
				SDL_Surface* tmp_conv_surf = SDL_ConvertSurfaceAndColorspace(surf, SDL_PIXELFORMAT_RGB24, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
				//auto end = Message::getTimeMS();
				// 1ms
				//std::cerr << "DTC: timing " << SDL_GetPixelFormatName(converted_surf->format) << "->SDL_PIXELFORMAT_RGB24: " << end-start << "ms\n";

				//start = Message::getTimeMS();
				surf = SDL_ConvertSurfaceAndColorspace(tmp_conv_surf, _forced_format, nullptr, SDL_COLORSPACE_YUV_DEFAULT, 0);
				//end = Message::getTimeMS();
				// 60ms
				//std::cerr << "DTC: timing SDL_PIXELFORMAT_RGB24->" << SDL_GetPixelFormatName(_forced_format) << ": " << end-start << "ms\n";

				SDL_DestroySurface(tmp_conv_surf);
			} else {
				surf = SDL_ConvertSurface(surf, _forced_format);
			}

			if (surf == nullptr) {
				// oh god
				std::cerr << "DTC error: failed to convert surface to IYUV: " << SDL_GetError() << "\n";
				return false;
			}
		}
		assert(surf != nullptr);
		if (surf != value.surface.get()) {
			// TODO: add ctr with uptr
			SDLVideoFrame new_value{value.timestampUS, nullptr};
			new_value.surface = {
				surf,
				&SDL_DestroySurface
			};

			return RealStream::push(std::move(new_value));
		} else {
			return RealStream::push(value);
		}
	}
};

