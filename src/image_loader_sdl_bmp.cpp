#include "./image_loader_sdl_bmp.hpp"

#include <SDL3/SDL.h>

#include <iostream>

ImageLoaderSDLBMP::ImageInfo ImageLoaderSDLBMP::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	auto* rw_ctx = SDL_RWFromConstMem(data, data_size);

	SDL_Surface* surf = SDL_LoadBMP_RW(rw_ctx, SDL_TRUE);
	if (surf == nullptr) {
		return res;
	}

	res.width = surf->w;
	res.height = surf->h;
	res.file_ext = "bmp";

	SDL_DestroySurface(surf);

	return res;
}

ImageLoaderSDLBMP::ImageResult ImageLoaderSDLBMP::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {
	ImageResult res;

	auto* rw_ctx = SDL_RWFromConstMem(data, data_size);

	SDL_Surface* surf = SDL_LoadBMP_RW(rw_ctx, SDL_TRUE);
	if (surf == nullptr) {
		return res;
	}

	SDL_Surface* conv_surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surf);
	if (conv_surf == nullptr) {
		return res;
	}

	res.width = surf->w;
	res.height = surf->h;
	res.file_ext = "bmp";

	SDL_LockSurface(conv_surf);

	auto& new_frame = res.frames.emplace_back();
	new_frame.ms = 0;
	new_frame.data.insert(new_frame.data.cbegin(), (const uint8_t*)conv_surf->pixels, ((const uint8_t*)conv_surf->pixels) + (surf->w*surf->h*4));

	SDL_UnlockSurface(conv_surf);
	SDL_DestroySurface(conv_surf);

	std::cout << "IL_SDLBMP: loaded img " << res.width << "x" << res.height << "\n";

	return res;
}

