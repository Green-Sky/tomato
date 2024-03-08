#pragma once

#include "./texture_uploader.hpp"

#include <SDL3/SDL.h>

struct SDLRendererTextureUploader : public TextureUploaderI {
	SDL_Renderer* renderer;
	SDLRendererTextureUploader(void) = delete;
	SDLRendererTextureUploader(SDL_Renderer* renderer_);
	~SDLRendererTextureUploader(void) = default;

	uint64_t uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter, Access access) override;
	bool updateRGBA(uint64_t tex_id, const uint8_t* data, size_t size) override;
	void destroy(uint64_t tex_id) override;
};

