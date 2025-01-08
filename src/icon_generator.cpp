#include "./icon_generator.hpp"

#include <qoi/qoi.h>
#include <cstdlib>

namespace {
#include "../res/icon/tomato_v1_256.qoi.h"
}

namespace IconGenerator {

SDL_Surface* base(void) {
	//ImageLoaderQOI{}.loadFromMemoryRGBA(tomato_v1_256_qoi, tomato_v1_256_qoi_len);

	qoi_desc desc;

	uint8_t* img_data = static_cast<uint8_t*>(
		qoi_decode(tomato_v1_256_qoi, tomato_v1_256_qoi_len, &desc, 4)
	);
	if (img_data == nullptr) {
		// not readable
		return nullptr;
	}

	auto* surf = SDL_CreateSurfaceFrom(desc.width, desc.height, SDL_PIXELFORMAT_RGBA32, img_data, 4*desc.width);
	if (surf == nullptr) {
		return nullptr;
	}

	auto* surf_dup = SDL_DuplicateSurface(surf);

	SDL_DestroySurface(surf);
	free(img_data);

	return surf_dup;
}

SDL_Surface* colorIndicator(float r, float g, float b, float a) {
	auto* surf = base();
	if (surf == nullptr) {
		return nullptr;
	}

	const SDL_Rect rect {
		// TODO: gaps and smaller
		// lower right quarter
		surf->w/2,
		surf->h/2,
		surf->w/2,
		surf->h/2,
	};

	const auto color = SDL_MapSurfaceRGBA(surf, r*255, g*255, b*255, a*255);

	SDL_FillSurfaceRect(surf, &rect, color);

	return surf;
}

} // IconGenerator

