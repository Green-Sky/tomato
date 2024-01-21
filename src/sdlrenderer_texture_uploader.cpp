#include "./sdlrenderer_texture_uploader.hpp"

#include <cassert>

SDLRendererTextureUploader::SDLRendererTextureUploader(SDL_Renderer* renderer_) :
	renderer(renderer_)
{
}

uint64_t SDLRendererTextureUploader::uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter) {
	// TODO: test if pitch is 4 or 4*width
	SDL_Surface* surf = SDL_CreateSurfaceFrom(
		(void*)data,
		width, height,
		4*width,
		SDL_PIXELFORMAT_RGBA32 // auto big/little
	);
	assert(surf); // TODO: add error reporting

	// TODO: this touches global state, reset?
	if (filter == NEAREST) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	} else if (filter == LINEAR) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	assert(tex); // TODO: add error reporting

	SDL_DestroySurface(surf);

	return reinterpret_cast<uint64_t>(tex);
}

void SDLRendererTextureUploader::destroy(uint64_t tex_id) {
	SDL_DestroyTexture(static_cast<SDL_Texture*>(reinterpret_cast<void*>(tex_id)));
}

