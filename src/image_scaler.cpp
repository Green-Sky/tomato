#include "./image_scaler.hpp"

#include <cmath>
#include <cassert>

// requires ColorTmp to have * and + operators

struct ColorCanvas8888;

struct ColorFloat4 {
	float v[4]{};

	ColorFloat4& operator*=(const float scalar) {
		v[0] *= scalar;
		v[1] *= scalar;
		v[2] *= scalar;
		v[3] *= scalar;
		return *this;
	}

	ColorFloat4 operator*(const float scalar) const {
		ColorFloat4 newcf = *this;
		newcf.v[0] *= scalar;
		newcf.v[1] *= scalar;
		newcf.v[2] *= scalar;
		newcf.v[3] *= scalar;
		return newcf;
	}

	ColorFloat4& operator/=(const float scalar) {
		v[0] /= scalar;
		v[1] /= scalar;
		v[2] /= scalar;
		v[3] /= scalar;
		return *this;
	}

	ColorFloat4& operator+=(const ColorFloat4& color) {
		v[0] += color.v[0];
		v[1] += color.v[1];
		v[2] += color.v[2];
		v[3] += color.v[3];
		return *this;
	}
};

struct ColorCanvas8888 {
	uint8_t* ptr {nullptr};

	ColorFloat4 operator[](size_t i) const {
		return {
			{
				float(ptr[i*4+0])/255.f,
				float(ptr[i*4+1])/255.f,
				float(ptr[i*4+2])/255.f,
				float(ptr[i*4+3])/255.f,
			}
		};
	}

	void set(size_t i, const ColorFloat4& color) {
		ptr[i*4+0] = std::round(color.v[0]*255.f);
		ptr[i*4+1] = std::round(color.v[1]*255.f);
		ptr[i*4+2] = std::round(color.v[2]*255.f);
		ptr[i*4+3] = std::round(color.v[3]*255.f);
	}
};

template<typename ColorCanvas, typename ColorTmp>
constexpr void image_scale(ColorCanvas& dst, const int dst_w, const int dst_h, const ColorCanvas& src, const int src_w, const int src_h) {
	// Box sampling - Imagine projecting the new, smaller pixels onto the larger source, covering multiple pixel.
	for (int y = 0; y < dst_h; y++) {
		for (int x = 0; x < dst_w; x++) {
			// We perform a weighted mean.
			ColorTmp color;
			float weight_sum = 0.f;

			// Walk from upper edge to bottom edge (vertical)
			const float edge_up = ((float)y * src_h) / dst_h;
			const float edge_down = ((y + 1.f) * src_h) / dst_h;
			for (float frac_pos_y = edge_up; frac_pos_y < edge_down;) {
				const int src_y = (int)std::floor(frac_pos_y); assert(src_y < src_h);
				const float frac_y = 1.f - (frac_pos_y - src_y);

				// Walk from left edge to right edge (horizontal)
				const float edge_left = ((float)x * src_w) / dst_w;
				const float edge_right = ((x + 1.f) * src_w) / dst_w;
				for (float frac_pos_x = edge_left; frac_pos_x < edge_right;) {
					const int src_x = (int)std::floor(frac_pos_x); assert(src_x < src_w);
					const float frac_x = 1.f - (frac_pos_x - src_x);

					const float src_pixel_weight = frac_x * frac_y;

					//const ColorTmp pixel_color = ImGui::ColorConvertU32ToFloat4(src[src_y * src_w + src_x]);
					const ColorTmp pixel_color = src[src_y * src_w + src_x];
					color += pixel_color * src_pixel_weight;
					weight_sum += src_pixel_weight;

					frac_pos_x += frac_x;
				}

				frac_pos_y += frac_y;
			}

			color /= weight_sum;

			dst.set(y * dst_w + x, color);
		}
	}
}

bool image_scale(uint8_t* dst, const int dst_w, const int dst_h, uint8_t* src, const int src_w, const int src_h) {
	if (dst == nullptr || src == nullptr) {
		return false;
	}
	if (dst_w == src_w && dst_h == src_h) {
		assert(false && "fix me !");
		return false;
	}

	ColorCanvas8888 dst_c{dst};
	const ColorCanvas8888 src_c{src};

	image_scale<ColorCanvas8888, ColorFloat4>(
		dst_c,
		dst_w, dst_h,
		src_c,
		src_w, src_h
	);

	return true;
}

bool image_scale(SDL_Surface* dst, SDL_Surface* src) {
	if (dst == nullptr || src == nullptr) {
		return false;
	}

	if (dst->format != src->format) {
		return false;
	}

	// TODO: handle other numbers of components beside 4

	if (
		src->format != SDL_PIXELFORMAT_RGBA8888 &&
		src->format != SDL_PIXELFORMAT_ARGB8888 &&
		src->format != SDL_PIXELFORMAT_BGRA8888 &&
		src->format != SDL_PIXELFORMAT_ABGR8888 &&
		src->format != SDL_PIXELFORMAT_RGBX8888 &&
		src->format != SDL_PIXELFORMAT_XRGB8888 &&
		src->format != SDL_PIXELFORMAT_BGRX8888 &&
		src->format != SDL_PIXELFORMAT_XBGR8888
	) {
		return false;
	}

	ColorCanvas8888 dst_c{reinterpret_cast<uint8_t*>(dst->pixels)};
	const ColorCanvas8888 src_c{reinterpret_cast<uint8_t*>(src->pixels)};

	image_scale<ColorCanvas8888, ColorFloat4>(
		dst_c,
		dst->w, dst->h,
		src_c,
		src->w, src->h
	);

	return true;
}

