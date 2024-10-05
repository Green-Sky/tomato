#pragma once

#include "./video.hpp"
#include "../frame_stream2.hpp"

#include <cassert>

#include <chrono> // meh
#include <iostream> // meh

template<typename RealStream>
struct PushConversionVideoStream : public RealStream {
	SDL_PixelFormat _forced_format {SDL_PIXELFORMAT_IYUV};
	// TODO: force colorspace?

	template<typename... Args>
	PushConversionVideoStream(SDL_PixelFormat forced_format, Args&&... args) : RealStream(std::forward<Args>(args)...), _forced_format(forced_format) {}
	~PushConversionVideoStream(void) {}

	bool push(const SDLVideoFrame& value) override {
		SDL_Surface* surf = value.surface.get();
		if (surf->format != _forced_format) {
			//std::cerr << "PCVS: need to convert from " << SDL_GetPixelFormatName(surf->format) << " to " << SDL_GetPixelFormatName(_forced_format) << "\n";
			//const auto start = std::chrono::steady_clock::now();
			if ((surf = SDL_ConvertSurface(surf, _forced_format)) == nullptr) {
				surf = value.surface.get(); // reset ptr
				//std::cerr << "PCVS warning: default conversion failed: " << SDL_GetError() << "\n";

				// sdl hardcodes BT709_LIMITED
				if ((surf = SDL_ConvertSurfaceAndColorspace(surf, _forced_format, nullptr, SDL_GetSurfaceColorspace(surf), 0)) == nullptr) {
				//if ((surf = SDL_ConvertSurfaceAndColorspace(surf, _forced_format, nullptr, SDL_COLORSPACE_BT709_LIMITED, 0)) == nullptr) {
					surf = value.surface.get(); // reset ptr
					//std::cerr << "PCVS warning: default conversion with same colorspace failed: " << SDL_GetError() << "\n";
					// simple convert failed, fall back to ->rgb->yuv
					//SDL_Surface* tmp_conv_surf = SDL_ConvertSurfaceAndColorspace(surf, SDL_PIXELFORMAT_RGB24, nullptr, SDL_COLORSPACE_RGB_DEFAULT, 0);
					SDL_Surface* tmp_conv_surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGB24);
					if (tmp_conv_surf == nullptr) {
						std::cerr << "PCVS error: conversion to RGB failed: " << SDL_GetError() << "\n";
					} else {
						//surf = SDL_ConvertSurfaceAndColorspace(tmp_conv_surf, _forced_format, nullptr, SDL_COLORSPACE_YUV_DEFAULT, 0);
						surf = SDL_ConvertSurface(tmp_conv_surf, _forced_format);
						//surf = SDL_ConvertSurfaceAndColorspace(tmp_conv_surf, _forced_format, nullptr, SDL_COLORSPACE_BT601_LIMITED, 0);
						SDL_DestroySurface(tmp_conv_surf);
					}
				}
			}
			//const auto end = std::chrono::steady_clock::now();
			//std::cerr << "PCVS: conversion took " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms\n";

			if (surf == nullptr) {
				// oh god
				std::cerr << "PCVS error: failed to convert surface to " << SDL_GetPixelFormatName(_forced_format) << ": " << SDL_GetError() << "\n";
				return false;
			}
			assert(surf != nullptr);

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

