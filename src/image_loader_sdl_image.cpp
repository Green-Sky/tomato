#include "./image_loader_sdl_image.hpp"

#include <SDL3_image/SDL_image.h>

#include <optional>
#include <iostream>

static std::optional<const char*> getExt(SDL_IOStream* ios) {
	if (IMG_isAVIF(ios)) {
		return "avif";
	} else if (IMG_isCUR(ios)) {
		return "cur";
	} else if (IMG_isICO(ios)) {
		return "ico";
	} else if (IMG_isBMP(ios)) {
		return "bmp";
	} else if (IMG_isGIF(ios)) {
		return "gif";
	} else if (IMG_isJPG(ios)) {
		return "jpg";
	} else if (IMG_isJXL(ios)) {
		return "jxl";
	} else if (IMG_isLBM(ios)) {
		return "lbm";
	} else if (IMG_isPCX(ios)) {
		return "pcx";
	} else if (IMG_isPNG(ios)) {
		return "png";
	} else if (IMG_isPNM(ios)) {
		return "pnm";
	} else if (IMG_isSVG(ios)) {
		return "svg";
	} else if (IMG_isTIF(ios)) {
		return "tiff";
	} else if (IMG_isXCF(ios)) {
		return "xcf";
	} else if (IMG_isXPM(ios)) {
		return "xpm";
	} else if (IMG_isXV(ios)) {
		return "xv";
	} else if (IMG_isWEBP(ios)) {
		return "webp";
	} else if (IMG_isQOI(ios)) {
		return "qoi";
	} else {
		return std::nullopt;
	}
}

ImageLoaderSDLImage::ImageInfo ImageLoaderSDLImage::loadInfoFromMemory(const uint8_t* data, uint64_t data_size) {
	ImageInfo res;

	auto* ios = SDL_IOFromConstMem(data, data_size);

	// we ignore tga
	auto ext_opt = getExt(ios);
	if (!ext_opt.has_value()) {
		return res;
	}

	SDL_Surface* surf = IMG_Load_IO(ios, true);
	if (surf == nullptr) {
		return res;
	}

	res.width = surf->w;
	res.height = surf->h;
	res.file_ext = ext_opt.value();

	SDL_DestroySurface(surf);

	return res;
}

ImageLoaderSDLImage::ImageResult ImageLoaderSDLImage::loadFromMemoryRGBA(const uint8_t* data, uint64_t data_size) {
	ImageResult res;

	auto* ios = SDL_IOFromConstMem(data, data_size);

	// we ignore tga
	auto ext_opt = getExt(ios);
	if (!ext_opt.has_value()) {
		return res;
	}

	IMG_Animation* anim = IMG_LoadAnimation_IO(ios, true);
	if (anim == nullptr) {
		return res;
	}

	for (int i = 0; i < anim->count; i++) {
		SDL_Surface* conv_surf = SDL_ConvertSurface(anim->frames[i], SDL_PIXELFORMAT_RGBA32);
		if (conv_surf == nullptr) {
			return res;
		}


		SDL_LockSurface(conv_surf);

		auto& new_frame = res.frames.emplace_back();
		new_frame.ms = anim->delays[i];
		new_frame.data = {(const uint8_t*)conv_surf->pixels, ((const uint8_t*)conv_surf->pixels) + (anim->w*anim->h*4)};

		SDL_UnlockSurface(conv_surf);
		SDL_DestroySurface(conv_surf);
	}

	res.width = anim->w;
	res.height = anim->h;
	res.file_ext = ext_opt.value();

	IMG_FreeAnimation(anim);

	std::cout << "IL_SDLI: loaded img " << res.width << "x" << res.height << "\n";

	return res;
}

