#include "./sdlrenderer_texture_uploader.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>

#include <iostream>

static SDL_PixelFormat format2sdl(TextureUploaderI::Format format) {
	switch (format) {
		case TextureUploaderI::Format::RGBA: return SDL_PIXELFORMAT_RGBA32;
		//case TextureUploaderI::Format::RGB: return SDL_PIXELFORMAT_RGB24;
		case TextureUploaderI::Format::IYUV: return SDL_PIXELFORMAT_IYUV;
		case TextureUploaderI::Format::YV12: return SDL_PIXELFORMAT_YV12;
		case TextureUploaderI::Format::NV12: return SDL_PIXELFORMAT_NV12;
		case TextureUploaderI::Format::NV21: return SDL_PIXELFORMAT_NV21;
		case TextureUploaderI::Format::MAX: return SDL_PIXELFORMAT_UNKNOWN;
	}
	return SDL_PIXELFORMAT_UNKNOWN;
}

SDLRendererTextureUploader::SDLRendererTextureUploader(SDL_Renderer* renderer_) :
	renderer(renderer_)
{
}

uint64_t SDLRendererTextureUploader::uploadRGBA(const uint8_t* data, uint32_t width, uint32_t height, Filter filter, Access access) {
	return upload(data, width, height, Format::RGBA, filter, access);
}

bool SDLRendererTextureUploader::updateRGBA(uint64_t tex_id, const uint8_t* data, size_t size) {
	return update(tex_id, data, size);
}

uint64_t SDLRendererTextureUploader::upload(const uint8_t* data, uint32_t width, uint32_t height, Format format, Filter filter, Access access) {
	const auto sdl_format = format2sdl(format);
	if (sdl_format == SDL_PIXELFORMAT_UNKNOWN) {
		std::cerr << "SDLRTU error: unsupported format '" << format << "'\n";
		return 0;
	}

	// TODO: why do we even create a non owning surface here???
	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surf = {
		SDL_CreateSurfaceFrom(
			width, height,
			sdl_format,
			(void*)data,
			4*width // ???
		),
		&SDL_DestroySurface
	};

	assert(surf);
	if (!surf) {
		std::cerr << "SDLRTU error: surf creation failed " << SDL_GetError() << "\n";
		return 0;
	}

	SDL_Texture* tex = SDL_CreateTexture(
		renderer,
		surf->format,
		access == Access::STREAMING ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC,
		surf->w, surf->h
	);

	assert(tex);
	if (tex == nullptr) {
		std::cerr << "SDLRTU error: tex creation failed " << SDL_GetError() << "\n";
		return 0;
	}

	bool need_to_lock = SDL_MUSTLOCK(surf);
	if (need_to_lock) {
		if (!SDL_LockSurface(surf.get())) {
			std::cerr << "SDLRTU error: failed to lock surface " << SDL_GetError() << "\n";
			SDL_DestroyTexture(tex);
			return 0;
		}
	}

	// while this split *should* not needed, the opengles renderer might like this more...
	if (sdl_format == SDL_PIXELFORMAT_IYUV || sdl_format == SDL_PIXELFORMAT_YV12) {
		if (!SDL_UpdateYUVTexture(
			tex,
			nullptr,
			static_cast<const uint8_t*>(surf->pixels),
			surf->w * 1,
			static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h,
			surf->w/2 * 1,
			static_cast<const uint8_t*>(surf->pixels) + (surf->w/2) * (surf->h/2),
			surf->w/2 * 1
		)) {
			std::cerr << "SDLRTU error: tex yuv update failed " << SDL_GetError() << "\n";
		}
	} else if (sdl_format == SDL_PIXELFORMAT_NV12 || sdl_format == SDL_PIXELFORMAT_NV21) {
		if (!SDL_UpdateNVTexture(
			tex,
			nullptr,
			static_cast<const uint8_t*>(surf->pixels),
			surf->w * 1,
			static_cast<const uint8_t*>(surf->pixels) + surf->w * surf->h,
			surf->w * 1
		)) {
			std::cerr << "SDLRTU error: tex nv update failed " << SDL_GetError() << "\n";
		}
	} else {
		if (!SDL_UpdateTexture(tex, nullptr, surf->pixels, surf->pitch)) {
			std::cerr << "SDLRTU error: tex update failed " << SDL_GetError() << "\n";
		}
	}

	if (need_to_lock) {
		// error check?
		SDL_UnlockSurface(surf.get());
	}

	SDL_BlendMode surf_blend_mode = SDL_BLENDMODE_NONE;
	if (SDL_GetSurfaceBlendMode(surf.get(), &surf_blend_mode)) {
		SDL_SetTextureBlendMode(tex, surf_blend_mode);
	}

	if (filter == NEAREST) {
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
	} else if (filter == LINEAR) {
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
	}

	return reinterpret_cast<uint64_t>(tex);
}

bool SDLRendererTextureUploader::update(uint64_t tex_id, const uint8_t* data, size_t size) {
	auto* texture = static_cast<SDL_Texture*>(reinterpret_cast<void*>(tex_id));
	if (texture == nullptr) {
		return false;
	}

	uint8_t* pixels = nullptr;
	int pitch = 0;

	if (!SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch)) {
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

