#include "./sdlrenderer_texture_uploader.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

SDLRendererTextureUploader::SDLRendererTextureUploader(SDL_Renderer* renderer_) :
	renderer(renderer_)
{
}

uint64_t SDLRendererTextureUploader::uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter, Access access) {
	// TODO: test if pitch is 4 or 4*width
	SDL_Surface* surf = SDL_CreateSurfaceFrom(
		(void*)data,
		width, height,
		4*width,
		SDL_PIXELFORMAT_RGBA32 // auto big/little
	);
	assert(surf); // TODO: add error reporting

	// hacky hint usage
	if (access == Access::STREAMING) {
		SDL_SetHint("SDL_TextureAccess", "SDL_TEXTUREACCESS_STREAMING");
	} else {
		SDL_SetHint("SDL_TextureAccess", "SDL_TEXTUREACCESS_STATIC");
	}
	// TODO: cleanup hints after

	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
	assert(tex); // TODO: add error reporting

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
		// TODO: error
		return false;
	}

	std::memcpy(pixels, data, size);

	SDL_UnlockTexture(texture);

	return true;
}

void SDLRendererTextureUploader::destroy(uint64_t tex_id) {
	SDL_DestroyTexture(static_cast<SDL_Texture*>(reinterpret_cast<void*>(tex_id)));
}

