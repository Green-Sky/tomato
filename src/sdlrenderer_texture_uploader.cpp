#include "./sdlrenderer_texture_uploader.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

#include <iostream>

SDLRendererTextureUploader::SDLRendererTextureUploader(SDL_Renderer* renderer_) :
	renderer(renderer_)
{
}

uint64_t SDLRendererTextureUploader::uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter, Access access) {
	// TODO: test if pitch is 4 or 4*width
	SDL_Surface* surf = SDL_CreateSurfaceFrom(
		width, height,
		SDL_PIXELFORMAT_RGBA32, // auto big/little
		(void*)data,
		4*width
	);
	assert(surf); // TODO: add error reporting

	SDL_Texture* tex = SDL_CreateTexture(
		renderer,
		surf->format,
		access == Access::STREAMING ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC,
		surf->w, surf->h
	);
	assert(tex); // TODO: add error reporting
	// TODO: error reporting
	SDL_UpdateTexture(tex, nullptr, surf->pixels, surf->pitch);

	SDL_BlendMode surf_blend_mode = SDL_BLENDMODE_NONE;
	if (SDL_GetSurfaceBlendMode(surf, &surf_blend_mode) == 0) {
		SDL_SetTextureBlendMode(tex, surf_blend_mode);
	}

	if (filter == NEAREST) {
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
	} else if (filter == LINEAR) {
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
	}

	SDL_DestroySurface(surf);

	return reinterpret_cast<uint64_t>(tex);
}

bool SDLRendererTextureUploader::updateRGBA(uint64_t tex_id, const uint8_t* data, size_t size) {
	auto* texture = static_cast<SDL_Texture*>(reinterpret_cast<void*>(tex_id));
	if (texture == nullptr) {
		return false;
	}

	uint8_t* pixels = nullptr;
	int pitch = 0;

	if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch) != 0) {
		std::cerr << "SDLRTU error: failed locking texture '" << SDL_GetError() << "'\n";
		return false;
	}

	std::memcpy(pixels, data, size);

	SDL_UnlockTexture(texture);

	return true;
}

void SDLRendererTextureUploader::destroy(uint64_t tex_id) {
	SDL_DestroyTexture(static_cast<SDL_Texture*>(reinterpret_cast<void*>(tex_id)));
}

