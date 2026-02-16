#include "./image_loader_sdl_bmp.hpp"

#include <SDL3/SDL.h>

#include <iostream>
#include <cassert>

ImageLoaderSDLBMP::ImageInfo ImageLoaderSDLBMP::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	auto* ios = SDL_IOFromConstMem(data, data_size);

	// frees ios for us
	SDL_Surface* surf = SDL_LoadBMP_IO(ios, true);
	if (surf == nullptr) {
		return res;
	}

	assert(surf->w >= 0);
	assert(surf->h >= 0);

	res.width = surf->w;
	res.height = surf->h;
	res.file_ext = "bmp";

	SDL_DestroySurface(surf);

	return res;
}

ImageLoaderSDLBMP::ImageResult ImageLoaderSDLBMP::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {

	auto* ios = SDL_IOFromConstMem(data, data_size);

	// frees ios for us
	SDL_Surface* surf = SDL_LoadBMP_IO(ios, true);
	if (surf == nullptr) {
		return {};
	}

	SDL_Surface* conv_surf = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surf);
	if (conv_surf == nullptr) {
		return {};
	}

	assert(conv_surf->w >= 0);
	assert(conv_surf->h >= 0);

	if (conv_surf->w > 16*1024 || conv_surf->h > 10*1024) {
		std::cerr << "IL_SDLBMP error: image too large\n";
		return {};
	}

	ImageResult res;
	if (SDL_MUSTLOCK(conv_surf)) {
		if (!SDL_LockSurface(conv_surf)) {
			std::cerr << "IL_SDLBMP error: " << SDL_GetError() << "\n";
			SDL_DestroySurface(conv_surf);
			return {};
		}
	}

	res.width = conv_surf->w;
	res.height = conv_surf->h;
	res.file_ext = "bmp";

	auto& new_frame = res.frames.emplace_back();
	new_frame.ms = 0;
	new_frame.data = {(const uint8_t*)conv_surf->pixels, ((const uint8_t*)conv_surf->pixels) + (conv_surf->w*conv_surf->h*4)};

	if (SDL_MUSTLOCK(conv_surf)) {
		SDL_UnlockSurface(conv_surf);
	}
	SDL_DestroySurface(conv_surf);

	std::cout << "IL_SDLBMP: loaded img " << res.width << "x" << res.height << "\n";

	return res;
}

