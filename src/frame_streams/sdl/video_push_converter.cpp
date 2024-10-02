#include "./video_push_converter.hpp"

#include <cstring>

SDL_Surface* convertYUY2_IYUV(SDL_Surface* surf) {
	if (surf->format != SDL_PIXELFORMAT_YUY2) {
		return nullptr;
	}
	if ((surf->w % 2) != 0) {
		SDL_SetError("YUY2->IYUV does not support odd widths");
		// hmmm, we dont handle odd widths
		return nullptr;
	}

	SDL_LockSurface(surf);

	SDL_Surface* conv_surf = SDL_CreateSurface(surf->w, surf->h, SDL_PIXELFORMAT_IYUV);
	SDL_LockSurface(conv_surf);

	// YUY2 is 4:2:2 packed
	// Y is simple, we just copy it over
	// U V are double the resolution (vertically), so we avg both
	// Packed mode: Y0+U0+Y1+V0 (1 plane)

	uint8_t* y_plane = static_cast<uint8_t*>(conv_surf->pixels);
	uint8_t* u_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h;
	uint8_t* v_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h + (conv_surf->w/2)*(conv_surf->h/2);

	const uint8_t* yuy2_data = static_cast<const uint8_t*>(surf->pixels);

	for (int y = 0; y < surf->h; y++) {
		for (int x = 0; x < surf->w; x += 2) {
			// every pixel uses 2 bytes
			const uint8_t* yuy2_curser = yuy2_data + y*surf->w*2 + x*2;
			uint8_t src_y0 = yuy2_curser[0];
			uint8_t src_u = yuy2_curser[1];
			uint8_t src_y1 = yuy2_curser[2];
			uint8_t src_v = yuy2_curser[3];

			y_plane[y*conv_surf->w + x] = src_y0;
			y_plane[y*conv_surf->w + x+1] = src_y1;

			size_t uv_index = (y/2) * (conv_surf->w/2) + x/2;
			if (y % 2 == 0) {
				// first write
				u_plane[uv_index] = src_u;
				v_plane[uv_index] = src_v;
			} else {
				// second write, mix with existing value
				u_plane[uv_index] = (int(u_plane[uv_index]) + int(src_u)) / 2;
				v_plane[uv_index] = (int(v_plane[uv_index]) + int(src_v)) / 2;
			}
		}
	}

	SDL_UnlockSurface(conv_surf);
	SDL_UnlockSurface(surf);
	return conv_surf;
}

SDL_Surface* convertNV12_IYUV(SDL_Surface* surf) {
	if (surf->format != SDL_PIXELFORMAT_NV12) {
		return nullptr;
	}
	if ((surf->w % 2) != 0) {
		SDL_SetError("NV12->IYUV does not support odd widths");
		// hmmm, we dont handle odd widths
		return nullptr;
	}

	SDL_LockSurface(surf);

	SDL_Surface* conv_surf = SDL_CreateSurface(surf->w, surf->h, SDL_PIXELFORMAT_IYUV);
	SDL_LockSurface(conv_surf);

	// NV12 is planar Y and interleaved UV in 4:2:0
	// Y is simple, we just copy it over
	// U V are ... packed?

	uint8_t* y_plane = static_cast<uint8_t*>(conv_surf->pixels);
	uint8_t* u_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h;
	uint8_t* v_plane = static_cast<uint8_t*>(conv_surf->pixels) + conv_surf->w*conv_surf->h + (conv_surf->w/2)*(conv_surf->h/2);

	const uint8_t* nv12_y_plane = static_cast<const uint8_t*>(surf->pixels);
	const uint8_t* nv12_uv_data = static_cast<const uint8_t*>(surf->pixels) + surf->w*surf->h;

	// y can be copied as is
	std::memcpy(
		y_plane,
		nv12_y_plane,
		surf->w*surf->h
	);

	// we operate at half res
	for (int y = 0; y < surf->h/2; y++) {
		for (int x = 0; x < surf->w/2; x++) {
			u_plane[y*(surf->w/2)+x] = nv12_uv_data[y*(surf->w/2)*2+x*2];
			v_plane[y*(surf->w/2)+x] = nv12_uv_data[y*(surf->w/2)*2+x*2+1];
		}
	}

	SDL_UnlockSurface(conv_surf);
	SDL_UnlockSurface(surf);
	return conv_surf;
}

