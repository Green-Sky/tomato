#include "./sdlrenderer_texture_uploader.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

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

	SDL_Texture* tex = SDL_CreateTexture(
		renderer,
		sdl_format,
		access == Access::STREAMING ? SDL_TEXTUREACCESS_STREAMING : SDL_TEXTUREACCESS_STATIC,
		width, height
	);

	assert(tex);
	if (tex == nullptr) {
		std::cerr << "SDLRTU error: tex creation failed " << SDL_GetError() << "\n";
		return 0;
	}

	// while this split *should* not be needed, the opengles renderer might like this more...
	if (sdl_format == SDL_PIXELFORMAT_IYUV || sdl_format == SDL_PIXELFORMAT_YV12) {
		const size_t pitch = width;
		const auto* yplane = data;
		const auto* uplane = yplane + pitch * height;
		const auto* vplane = uplane + ((pitch+1)/2) * ((height+1)/2);
		if (!SDL_UpdateYUVTexture(
			tex,
			nullptr,
			yplane, pitch,
			uplane, (pitch+1)/2,
			vplane, (pitch+1)/2
		)) {
			std::cerr << "SDLRTU error: tex yuv update failed " << SDL_GetError() << "\n";
		}
	} else if (sdl_format == SDL_PIXELFORMAT_NV12 || sdl_format == SDL_PIXELFORMAT_NV21) {
		const size_t pitch = width;
		const auto* yplane = data;
		const auto* uvplane = data + pitch * height;
		if (!SDL_UpdateNVTexture(
			tex,
			nullptr,
			yplane, pitch,
			uvplane, pitch
		)) {
			std::cerr << "SDLRTU error: tex nv update failed " << SDL_GetError() << "\n";
		}
	} else {
		assert(format == Format::RGBA);
		const size_t pitch = width*4;
		if (!SDL_UpdateTexture(tex, nullptr, data, pitch)) {
			std::cerr << "SDLRTU error: tex update failed " << SDL_GetError() << "\n";
		}
#if 0
		SDL_BlendMode surf_blend_mode = SDL_BLENDMODE_NONE;
		if (SDL_GetSurfaceBlendMode(surf.get(), &surf_blend_mode)) {
			SDL_SetTextureBlendMode(tex, surf_blend_mode);
		}
#endif
		// TODO: if alpha comp, enable blend, or just always?
		SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
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

