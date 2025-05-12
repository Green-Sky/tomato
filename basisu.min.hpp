#pragma once

#include <algorithm>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <vector>

namespace basisu
{

	using std::clamp;
	using std::min;
	using std::max;

	template <typename T0, typename T1> inline T0 lerp(T0 a, T0 b, T1 c) { return a + (b - a) * c; }


	class color_rgba
	{
	public:
		union
		{
			uint8_t m_comps[4];

			struct
			{
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};
		};

		inline color_rgba()
		{
			static_assert(sizeof(*this) == 4, "sizeof(*this) != 4");
		}

		inline color_rgba(int y)
		{
			set(y);
		}

		inline color_rgba(int y, int na)
		{
			set(y, na);
		}

		inline color_rgba(int sr, int sg, int sb, int sa)
		{
			set(sr, sg, sb, sa);
		}

		//inline color_rgba(eNoClamp, int sr, int sg, int sb, int sa)
		//{
		//    set_noclamp_rgba((uint8_t)sr, (uint8_t)sg, (uint8_t)sb, (uint8_t)sa);
		//}

		inline color_rgba& set_noclamp_y(int y)
		{
			m_comps[0] = (uint8_t)y;
			m_comps[1] = (uint8_t)y;
			m_comps[2] = (uint8_t)y;
			m_comps[3] = (uint8_t)255;
			return *this;
		}

		inline color_rgba &set_noclamp_rgba(int sr, int sg, int sb, int sa)
		{
			m_comps[0] = (uint8_t)sr;
			m_comps[1] = (uint8_t)sg;
			m_comps[2] = (uint8_t)sb;
			m_comps[3] = (uint8_t)sa;
			return *this;
		}

		inline color_rgba &set(int y)
		{
			m_comps[0] = static_cast<uint8_t>(clamp<int>(y, 0, 255));
			m_comps[1] = m_comps[0];
			m_comps[2] = m_comps[0];
			m_comps[3] = 255;
			return *this;
		}

		inline color_rgba &set(int y, int na)
		{
			m_comps[0] = static_cast<uint8_t>(clamp<int>(y, 0, 255));
			m_comps[1] = m_comps[0];
			m_comps[2] = m_comps[0];
			m_comps[3] = static_cast<uint8_t>(clamp<int>(na, 0, 255));
			return *this;
		}

		inline color_rgba &set(int sr, int sg, int sb, int sa)
		{
			m_comps[0] = static_cast<uint8_t>(clamp<int>(sr, 0, 255));
			m_comps[1] = static_cast<uint8_t>(clamp<int>(sg, 0, 255));
			m_comps[2] = static_cast<uint8_t>(clamp<int>(sb, 0, 255));
			m_comps[3] = static_cast<uint8_t>(clamp<int>(sa, 0, 255));
			return *this;
		}

		inline color_rgba &set_rgb(int sr, int sg, int sb)
		{
			m_comps[0] = static_cast<uint8_t>(clamp<int>(sr, 0, 255));
			m_comps[1] = static_cast<uint8_t>(clamp<int>(sg, 0, 255));
			m_comps[2] = static_cast<uint8_t>(clamp<int>(sb, 0, 255));
			return *this;
		}

		inline color_rgba &set_rgb(const color_rgba &other)
		{
			r = other.r;
			g = other.g;
			b = other.b;
			return *this;
		}

		inline const uint8_t &operator[] (uint32_t index) const { assert(index < 4); return m_comps[index]; }
		inline uint8_t &operator[] (uint32_t index) { assert(index < 4); return m_comps[index]; }

		inline void clear()
		{
			m_comps[0] = 0;
			m_comps[1] = 0;
			m_comps[2] = 0;
			m_comps[3] = 0;
		}

		inline bool operator== (const color_rgba &rhs) const
		{
			if (m_comps[0] != rhs.m_comps[0]) return false;
			if (m_comps[1] != rhs.m_comps[1]) return false;
			if (m_comps[2] != rhs.m_comps[2]) return false;
			if (m_comps[3] != rhs.m_comps[3]) return false;
			return true;
		}

		inline bool operator!= (const color_rgba &rhs) const
		{
			return !(*this == rhs);
		}

		inline bool operator<(const color_rgba &rhs) const
		{
			for (int i = 0; i < 4; i++)
			{
				if (m_comps[i] < rhs.m_comps[i])
					return true;
				else if (m_comps[i] != rhs.m_comps[i])
					return false;
			}
			return false;
		}

		inline int get_601_luma() const { return (19595U * m_comps[0] + 38470U * m_comps[1] + 7471U * m_comps[2] + 32768U) >> 16U; }
		inline int get_709_luma() const { return (13938U * m_comps[0] + 46869U * m_comps[1] + 4729U * m_comps[2] + 32768U) >> 16U; }
		inline int get_luma(bool luma_601) const { return luma_601 ? get_601_luma() : get_709_luma(); }

		static color_rgba comp_min(const color_rgba& a, const color_rgba& b) { return color_rgba(min(a[0], b[0]), min(a[1], b[1]), min(a[2], b[2]), min(a[3], b[3])); }
		static color_rgba comp_max(const color_rgba& a, const color_rgba& b) { return color_rgba(max(a[0], b[0]), max(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3])); }
	};

	typedef std::vector<color_rgba> color_rgba_vec;

	const color_rgba g_black_color(0, 0, 0, 255);
	const color_rgba g_black_trans_color(0, 0, 0, 0);
	const color_rgba g_white_color(255, 255, 255, 255);

	// Simple 32-bit 2D image class

	class image
	{
	public:
		image() :
			m_width(0), m_height(0), m_pitch(0)
		{
		}

		image(uint32_t w, uint32_t h, uint32_t p = UINT32_MAX) :
			m_width(0), m_height(0), m_pitch(0)
		{
			resize(w, h, p);
		}

		image(const uint8_t *pImage, uint32_t width, uint32_t height, uint32_t comps) :
			m_width(0), m_height(0), m_pitch(0)
		{
			init(pImage, width, height, comps);
		}

		image(const image &other) :
			m_width(0), m_height(0), m_pitch(0)
		{
			*this = other;
		}

		image &swap(image &other)
		{
			std::swap(m_width, other.m_width);
			std::swap(m_height, other.m_height);
			std::swap(m_pitch, other.m_pitch);
			m_pixels.swap(other.m_pixels);
			return *this;
		}

		image &operator= (const image &rhs)
		{
			if (this != &rhs)
			{
				m_width = rhs.m_width;
				m_height = rhs.m_height;
				m_pitch = rhs.m_pitch;
				m_pixels = rhs.m_pixels;
			}
			return *this;
		}

		image &clear()
		{
			m_width = 0;
			m_height = 0;
			m_pitch = 0;
			m_pixels.erase(m_pixels.begin(), m_pixels.end());
			return *this;
		}

		image &resize(uint32_t w, uint32_t h, uint32_t p = UINT32_MAX, const color_rgba& background = g_black_color)
		{
			return crop(w, h, p, background);
		}

		image &set_all(const color_rgba &c)
		{
			for (uint32_t i = 0; i < m_pixels.size(); i++)
				m_pixels[i] = c;
			return *this;
		}

		void init(const uint8_t *pImage, uint32_t width, uint32_t height, uint32_t comps)
		{
			assert(comps >= 1 && comps <= 4);

			resize(width, height);

			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x++)
				{
					const uint8_t *pSrc = &pImage[(x + y * width) * comps];
					color_rgba &dst = (*this)(x, y);

					if (comps == 1)
					{
						dst.r = pSrc[0];
						dst.g = pSrc[0];
						dst.b = pSrc[0];
						dst.a = 255;
					}
					else if (comps == 2)
					{
						dst.r = pSrc[0];
						dst.g = pSrc[0];
						dst.b = pSrc[0];
						dst.a = pSrc[1];
					}
					else
					{
						dst.r = pSrc[0];
						dst.g = pSrc[1];
						dst.b = pSrc[2];
						if (comps == 4)
							dst.a = pSrc[3];
						else
							dst.a = 255;
					}
				}
			}
		}

		image &fill_box(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const color_rgba &c)
		{
			for (uint32_t iy = 0; iy < h; iy++)
				for (uint32_t ix = 0; ix < w; ix++)
					set_clipped(x + ix, y + iy, c);
			return *this;
		}

		image& fill_box_alpha(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const color_rgba& c)
		{
			for (uint32_t iy = 0; iy < h; iy++)
				for (uint32_t ix = 0; ix < w; ix++)
					set_clipped_alpha(x + ix, y + iy, c);
			return *this;
		}

		image &crop_dup_borders(uint32_t w, uint32_t h)
		{
			const uint32_t orig_w = m_width, orig_h = m_height;

			crop(w, h);

			if (orig_w && orig_h)
			{
				if (m_width > orig_w)
				{
					for (uint32_t x = orig_w; x < m_width; x++)
						for (uint32_t y = 0; y < m_height; y++)
							set_clipped(x, y, get_clamped(min(x, orig_w - 1U), min(y, orig_h - 1U)));
				}

				if (m_height > orig_h)
				{
					for (uint32_t y = orig_h; y < m_height; y++)
						for (uint32_t x = 0; x < m_width; x++)
							set_clipped(x, y, get_clamped(min(x, orig_w - 1U), min(y, orig_h - 1U)));
				}
			}
			return *this;
		}

		//// pPixels MUST have been allocated using malloc() (basisu::vector will eventually use free() on the pointer).
		//image& grant_ownership(color_rgba* pPixels, uint32_t w, uint32_t h, uint32_t p = UINT32_MAX)
		//{
		//    if (p == UINT32_MAX)
		//        p = w;

		//    clear();

		//    if ((!p) || (!w) || (!h))
		//        return *this;

		//    m_pixels.grant_ownership(pPixels, p * h, p * h);

		//    m_width = w;
		//    m_height = h;
		//    m_pitch = p;

		//    return *this;
		//}

		image &crop(uint32_t w, uint32_t h, uint32_t p = UINT32_MAX, const color_rgba &background = g_black_color, bool init_image = true)
		{
			if (p == UINT32_MAX)
				p = w;

			if ((w == m_width) && (m_height == h) && (m_pitch == p))
				return *this;

			if ((!w) || (!h) || (!p))
			{
				clear();
				return *this;
			}

			color_rgba_vec cur_state;
			cur_state.swap(m_pixels);

			m_pixels.resize(p * h);

			if (init_image)
			{
				if (m_width || m_height)
				{
					for (uint32_t y = 0; y < h; y++)
					{
						for (uint32_t x = 0; x < w; x++)
						{
							if ((x < m_width) && (y < m_height))
								m_pixels[x + y * p] = cur_state[x + y * m_pitch];
							else
								m_pixels[x + y * p] = background;
						}
					}
				}
				else
				{
					//m_pixels.set_all(background);
					set_all(background);
				}
			}

			m_width = w;
			m_height = h;
			m_pitch = p;

			return *this;
		}

		inline const color_rgba &operator() (uint32_t x, uint32_t y) const { assert(x < m_width && y < m_height); return m_pixels[x + y * m_pitch]; }
		inline color_rgba &operator() (uint32_t x, uint32_t y) { assert(x < m_width && y < m_height); return m_pixels[x + y * m_pitch]; }

		inline const color_rgba& get_pixel(uint32_t c) const { return (*this)(c % m_width, c / m_width); }
		inline color_rgba& get_pixel(uint32_t c) { return (*this)(c % m_width, c / m_width); }

		inline const color_rgba &get_clamped(int x, int y) const { return (*this)(clamp<int>(x, 0, m_width - 1), clamp<int>(y, 0, m_height - 1)); }
		inline color_rgba &get_clamped(int x, int y) { return (*this)(clamp<int>(x, 0, m_width - 1), clamp<int>(y, 0, m_height - 1)); }

		//inline const color_rgba &get_clamped_or_wrapped(int x, int y, bool wrap_u, bool wrap_v) const
		//{
		//    x = wrap_u ? posmod(x, m_width) : clamp<int>(x, 0, m_width - 1);
		//    y = wrap_v ? posmod(y, m_height) : clamp<int>(y, 0, m_height - 1);
		//    return m_pixels[x + y * m_pitch];
		//}

		//inline color_rgba &get_clamped_or_wrapped(int x, int y, bool wrap_u, bool wrap_v)
		//{
		//    x = wrap_u ? posmod(x, m_width) : clamp<int>(x, 0, m_width - 1);
		//    y = wrap_v ? posmod(y, m_height) : clamp<int>(y, 0, m_height - 1);
		//    return m_pixels[x + y * m_pitch];
		//}

		inline image &set_clipped(int x, int y, const color_rgba &c)
		{
			if ((static_cast<uint32_t>(x) < m_width) && (static_cast<uint32_t>(y) < m_height))
				(*this)(x, y) = c;
			return *this;
		}

		inline image& set_clipped_alpha(int x, int y, const color_rgba& c)
		{
			if ((static_cast<uint32_t>(x) < m_width) && (static_cast<uint32_t>(y) < m_height))
				(*this)(x, y).m_comps[3] = c.m_comps[3];
			return *this;
		}

		// Very straightforward blit with full clipping. Not fast, but it works.
		image &blit(const image &src, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y)
		{
			for (int y = 0; y < src_h; y++)
			{
				const int sy = src_y + y;
				if (sy < 0)
					continue;
				else if (sy >= (int)src.get_height())
					break;

				for (int x = 0; x < src_w; x++)
				{
					const int sx = src_x + x;
					if (sx < 0)
						continue;
					else if (sx >= (int)src.get_height())
						break;

					set_clipped(dst_x + x, dst_y + y, src(sx, sy));
				}
			}

			return *this;
		}

		const image &extract_block_clamped(color_rgba *pDst, uint32_t src_x, uint32_t src_y, uint32_t w, uint32_t h) const
		{
			if (((src_x + w) > m_width) || ((src_y + h) > m_height))
			{
				// Slower clamping case
				for (uint32_t y = 0; y < h; y++)
					for (uint32_t x = 0; x < w; x++)
						*pDst++ = get_clamped(src_x + x, src_y + y);
			}
			else
			{
				const color_rgba* pSrc = &m_pixels[src_x + src_y * m_pitch];

				for (uint32_t y = 0; y < h; y++)
				{
					std::memcpy(pDst, pSrc, w * sizeof(color_rgba));
					pSrc += m_pitch;
					pDst += w;
				}
			}

			return *this;
		}

		image &set_block_clipped(const color_rgba *pSrc, uint32_t dst_x, uint32_t dst_y, uint32_t w, uint32_t h)
		{
			for (uint32_t y = 0; y < h; y++)
				for (uint32_t x = 0; x < w; x++)
					set_clipped(dst_x + x, dst_y + y, *pSrc++);
			return *this;
		}

		inline uint32_t get_width() const { return m_width; }
		inline uint32_t get_height() const { return m_height; }
		inline uint32_t get_pitch() const { return m_pitch; }
		inline uint32_t get_total_pixels() const { return m_width * m_height; }

		inline uint32_t get_block_width(uint32_t w) const { return (m_width + (w - 1)) / w; }
		inline uint32_t get_block_height(uint32_t h) const { return (m_height + (h - 1)) / h; }
		inline uint32_t get_total_blocks(uint32_t w, uint32_t h) const { return get_block_width(w) * get_block_height(h); }

		inline const color_rgba_vec &get_pixels() const { return m_pixels; }
		inline color_rgba_vec &get_pixels() { return m_pixels; }

		inline const color_rgba *get_ptr() const { return &m_pixels[0]; }
		inline color_rgba *get_ptr() { return &m_pixels[0]; }

		bool has_alpha() const
		{
			for (uint32_t y = 0; y < m_height; ++y)
				for (uint32_t x = 0; x < m_width; ++x)
					if ((*this)(x, y).a < 255)
						return true;

			return false;
		}

		image &set_alpha(uint8_t a)
		{
			for (uint32_t y = 0; y < m_height; ++y)
				for (uint32_t x = 0; x < m_width; ++x)
					(*this)(x, y).a = a;
			return *this;
		}

		image &flip_y()
		{
			for (uint32_t y = 0; y < m_height / 2; ++y)
				for (uint32_t x = 0; x < m_width; ++x)
					std::swap((*this)(x, y), (*this)(x, m_height - 1 - y));
			return *this;
		}

		//// TODO: There are many ways to do this, not sure this is the best way.
		//image &renormalize_normal_map()
		//{
		//    for (uint32_t y = 0; y < m_height; y++)
		//    {
		//        for (uint32_t x = 0; x < m_width; x++)
		//        {
		//            color_rgba &c = (*this)(x, y);
		//            if ((c.r == 128) && (c.g == 128) && (c.b == 128))
		//                continue;

		//            vec3F v(c.r, c.g, c.b);
		//            v = (v * (2.0f / 255.0f)) - vec3F(1.0f);
		//            v.clamp(-1.0f, 1.0f);

		//            float length = v.length();
		//            const float cValidThresh = .077f;
		//            if (length < cValidThresh)
		//            {
		//                c.set(128, 128, 128, c.a);
		//            }
		//            else if (fabs(length - 1.0f) > cValidThresh)
		//            {
		//                if (length)
		//                    v /= length;

		//                for (uint32_t i = 0; i < 3; i++)
		//                    c[i] = static_cast<uint8_t>(clamp<float>(floor((v[i] + 1.0f) * 255.0f * .5f + .5f), 0.0f, 255.0f));

		//                if ((c.g == 128) && (c.r == 128))
		//                {
		//                    if (c.b < 128)
		//                        c.b = 0;
		//                    else
		//                        c.b = 255;
		//                }
		//            }
		//        }
		//    }
		//    return *this;
		//}

		bool operator== (const image& img) const
		{
			if ((m_width != img.get_width()) || (m_height != img.get_height()))
				return false;

			for (uint32_t y = 0; y < m_height; y++)
				for (uint32_t x = 0; x < m_width; x++)
					if ((*this)(x, y) != img(x, y))
						return false;

			return true;
		}

		bool operator!= (const image& img) const
		{
			return !(*this == img);
		}

		void debug_text(uint32_t x_ofs, uint32_t y_ofs, uint32_t x_scale, uint32_t y_scale, const color_rgba &fg, const color_rgba *pBG, bool alpha_only, const char* p, ...);

	private:
		uint32_t m_width, m_height, m_pitch;  // all in pixels
		color_rgba_vec m_pixels;
	};

	enum eZero { cZero };

	// Linear algebra

	template <uint32_t N, typename T>
	class vec
	{
	protected:
		T m_v[N];

	public:
		enum { num_elements = N };

		inline vec() { }
		inline vec(eZero) { set_zero();  }

		explicit inline vec(T val) { set(val); }
		inline vec(T v0, T v1) { set(v0, v1); }
		inline vec(T v0, T v1, T v2) { set(v0, v1, v2); }
		inline vec(T v0, T v1, T v2, T v3) { set(v0, v1, v2, v3); }
		inline vec(const vec &other) { for (uint32_t i = 0; i < N; i++) m_v[i] = other.m_v[i]; }
		template <uint32_t OtherN, typename OtherT> inline vec(const vec<OtherN, OtherT> &other) { set(other); }

		inline T operator[](uint32_t i) const { assert(i < N); return m_v[i]; }
		inline T &operator[](uint32_t i) { assert(i < N); return m_v[i]; }

		inline T getX() const { return m_v[0]; }
		inline T getY() const { static_assert(N >= 2, "N too small"); return m_v[1]; }
		inline T getZ() const { static_assert(N >= 3, "N too small"); return m_v[2]; }
		inline T getW() const { static_assert(N >= 4, "N too small"); return m_v[3]; }

		inline bool operator==(const vec &rhs) const { for (uint32_t i = 0; i < N; i++) if (m_v[i] != rhs.m_v[i]) return false;	return true; }
		inline bool operator<(const vec &rhs) const { for (uint32_t i = 0; i < N; i++) { if (m_v[i] < rhs.m_v[i]) return true; else if (m_v[i] != rhs.m_v[i]) return false; } return false; }

		inline void set_zero() { for (uint32_t i = 0; i < N; i++) m_v[i] = 0; }

		template <uint32_t OtherN, typename OtherT>
		inline vec &set(const vec<OtherN, OtherT> &other)
		{
			uint32_t i;
			if ((const void *)(&other) == (const void *)(this))
				return *this;
			const uint32_t m = min(OtherN, N);
			for (i = 0; i < m; i++)
				m_v[i] = static_cast<T>(other[i]);
			for (; i < N; i++)
				m_v[i] = 0;
			return *this;
		}

		inline vec &set_component(uint32_t index, T val) { assert(index < N); m_v[index] = val; return *this; }
		inline vec &set(T val) { for (uint32_t i = 0; i < N; i++) m_v[i] = val; return *this; }
		inline void clear_elements(uint32_t s, uint32_t e) { assert(e <= N); for (uint32_t i = s; i < e; i++) m_v[i] = 0; }

		inline vec &set(T v0, T v1)
		{
			m_v[0] = v0;
			if (N >= 2)
			{
				m_v[1] = v1;
				clear_elements(2, N);
			}
			return *this;
		}

		inline vec &set(T v0, T v1, T v2)
		{
			m_v[0] = v0;
			if (N >= 2)
			{
				m_v[1] = v1;
				if (N >= 3)
				{
					m_v[2] = v2;
					clear_elements(3, N);
				}
			}
			return *this;
		}

		inline vec &set(T v0, T v1, T v2, T v3)
		{
			m_v[0] = v0;
			if (N >= 2)
			{
				m_v[1] = v1;
				if (N >= 3)
				{
					m_v[2] = v2;

					if (N >= 4)
					{
						m_v[3] = v3;
						clear_elements(5, N);
					}
				}
			}
			return *this;
		}

		inline vec &operator=(const vec &rhs) { if (this != &rhs) for (uint32_t i = 0; i < N; i++) m_v[i] = rhs.m_v[i]; return *this; }
		template <uint32_t OtherN, typename OtherT> inline vec &operator=(const vec<OtherN, OtherT> &rhs) { set(rhs); return *this; }

		inline const T *get_ptr() const { return reinterpret_cast<const T *>(&m_v[0]); }
		inline T *get_ptr() { return reinterpret_cast<T *>(&m_v[0]); }

		inline vec operator- () const { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = -m_v[i]; return res; }
		inline vec operator+ () const { return *this; }
		inline vec &operator+= (const vec &other) { for (uint32_t i = 0; i < N; i++) m_v[i] += other.m_v[i]; return *this; }
		inline vec &operator-= (const vec &other) { for (uint32_t i = 0; i < N; i++) m_v[i] -= other.m_v[i]; return *this; }
		inline vec &operator/= (const vec &other) { for (uint32_t i = 0; i < N; i++) m_v[i] /= other.m_v[i]; return *this; }
		inline vec &operator*=(const vec &other) { for (uint32_t i = 0; i < N; i++) m_v[i] *= other.m_v[i]; return *this; }
		inline vec &operator/= (T s) { for (uint32_t i = 0; i < N; i++) m_v[i] /= s; return *this; }
		inline vec &operator*= (T s) { for (uint32_t i = 0; i < N; i++) m_v[i] *= s; return *this; }

		friend inline vec operator+(const vec &lhs, const vec &rhs) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = lhs.m_v[i] + rhs.m_v[i]; return res; }
		friend inline vec operator-(const vec &lhs, const vec &rhs) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = lhs.m_v[i] - rhs.m_v[i]; return res; }
		friend inline vec operator*(const vec &lhs, T val) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = lhs.m_v[i] * val; return res; }
		friend inline vec operator*(T val, const vec &rhs) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = val * rhs.m_v[i]; return res; }
		friend inline vec operator/(const vec &lhs, T val) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = lhs.m_v[i] / val; return res; }
		friend inline vec operator/(const vec &lhs, const vec &rhs) { vec res; for (uint32_t i = 0; i < N; i++) res.m_v[i] = lhs.m_v[i] / rhs.m_v[i]; return res; }

		static inline T dot_product(const vec &lhs, const vec &rhs) { T res = lhs.m_v[0] * rhs.m_v[0]; for (uint32_t i = 1; i < N; i++) res += lhs.m_v[i] * rhs.m_v[i]; return res; }

		inline T dot(const vec &rhs) const { return dot_product(*this, rhs); }

		inline T norm() const { return dot_product(*this, *this); }
		inline T length() const { return sqrt(norm()); }

		inline T squared_distance(const vec &other) const { T d2 = 0; for (uint32_t i = 0; i < N; i++) { T d = m_v[i] - other.m_v[i]; d2 += d * d; } return d2; }
		inline double squared_distance_d(const vec& other) const { double d2 = 0; for (uint32_t i = 0; i < N; i++) { double d = (double)m_v[i] - (double)other.m_v[i]; d2 += d * d; } return d2; }

		inline T distance(const vec &other) const { return static_cast<T>(sqrt(squared_distance(other))); }
		inline double distance_d(const vec& other) const { return sqrt(squared_distance_d(other)); }

		inline vec &normalize_in_place() { T len = length(); if (len != 0.0f) *this *= (1.0f / len);	return *this; }

		inline vec &clamp(T l, T h)
		{
			for (uint32_t i = 0; i < N; i++)
				m_v[i] = basisu::clamp(m_v[i], l, h);
			return *this;
		}

		static vec component_min(const vec& a, const vec& b)
		{
			vec res;
			for (uint32_t i = 0; i < N; i++)
				res[i] = min(a[i], b[i]);
			return res;
		}

		static vec component_max(const vec& a, const vec& b)
		{
			vec res;
			for (uint32_t i = 0; i < N; i++)
				res[i] = max(a[i], b[i]);
			return res;
		}
	};

	typedef vec<4, double> vec4D;
	typedef vec<3, double> vec3D;
	typedef vec<2, double> vec2D;
	typedef vec<1, double> vec1D;

	typedef vec<4, float> vec4F;
	typedef vec<3, float> vec3F;
	typedef vec<2, float> vec2F;
	typedef vec<1, float> vec1F;

	typedef vec<16, float> vec16F;

	// 2D array

	template<typename T>
	class vector2D
	{
		typedef std::vector<T> TVec;

		uint32_t m_width, m_height;
		TVec m_values;

	public:
		vector2D() :
			m_width(0),
			m_height(0)
		{
		}

		vector2D(uint32_t w, uint32_t h) :
			m_width(0),
			m_height(0)
		{
			resize(w, h);
		}

		vector2D(const vector2D &other)
		{
			*this = other;
		}

		vector2D &operator= (const vector2D &other)
		{
			if (this != &other)
			{
				m_width = other.m_width;
				m_height = other.m_height;
				m_values = other.m_values;
			}
			return *this;
		}

		inline bool operator== (const vector2D &rhs) const
		{
			return (m_width == rhs.m_width) && (m_height == rhs.m_height) && (m_values == rhs.m_values);
		}

		inline uint32_t size_in_bytes() const { return (uint32_t)m_values.size() * sizeof(m_values[0]); }

		inline const T &operator() (uint32_t x, uint32_t y) const { assert(x < m_width && y < m_height); return m_values[x + y * m_width]; }
		inline T &operator() (uint32_t x, uint32_t y) { assert(x < m_width && y < m_height); return m_values[x + y * m_width]; }

		inline const T &operator[] (uint32_t i) const { return m_values[i]; }
		inline T &operator[] (uint32_t i) { return m_values[i]; }

		inline const T &at_clamped(int x, int y) const { return (*this)(clamp<int>(x, 0, m_width), clamp<int>(y, 0, m_height)); }
		inline T &at_clamped(int x, int y) { return (*this)(clamp<int>(x, 0, m_width), clamp<int>(y, 0, m_height)); }

		void clear()
		{
			m_width = 0;
			m_height = 0;
			m_values.clear();
		}

		void set_all(const T&val)
		{
			//vector_set_all(m_values, val);
			for (size_t i = 0; i < m_values.size(); i++)
				m_values[i] = val;
		}

		inline const T* get_ptr() const { return &m_values[0]; }
		inline T* get_ptr() { return &m_values[0]; }

		vector2D &resize(uint32_t new_width, uint32_t new_height)
		{
			if ((m_width == new_width) && (m_height == new_height))
				return *this;

			TVec oldVals(new_width * new_height);
			oldVals.swap(m_values);

			const uint32_t w = min(m_width, new_width);
			const uint32_t h = min(m_height, new_height);

			if ((w) && (h))
			{
				for (uint32_t y = 0; y < h; y++)
					for (uint32_t x = 0; x < w; x++)
						m_values[x + y * new_width] = oldVals[x + y * m_width];
			}

			m_width = new_width;
			m_height = new_height;

			return *this;
		}
	};

} // basisu

