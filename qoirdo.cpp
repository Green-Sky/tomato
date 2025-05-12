// qoirdo.cpp
// Copyright (C) 2022 Richard Geldreich, Jr. All Rights Reserved.
// Copyright (C) 2025 Erik Scholz
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "./qoirdo.hpp"

#if _MSC_VER
// For sprintf(), strcpy()
#define _CRT_SECURE_NO_WARNINGS (1)
#endif

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <string>
#include <array>
#include <vector>

#include "./basisu.min.hpp"

using namespace basisu;

#define RDO_PNG_VERSION "v1.10"

const float DEF_MAX_SMOOTH_STD_DEV = 35.0f;
const float DEF_SMOOTH_MAX_MSE_SCALE = 250.0f;
const float DEF_MAX_ULTRA_SMOOTH_STD_DEV = 5.0F;
const float DEF_ULTRA_SMOOTH_MAX_MSE_SCALE = 1500.0F;

const float QOI_DEF_SMOOTH_MAX_MSE_SCALE = 2500.0f;
const float QOI_DEF_ULTRA_SMOOTH_MAX_MSE_SCALE = 5000.0f;

enum speed_mode
{
	cNormalSpeed,
	cFasterSpeed,
	cFastestSpeed
};

struct rdo_png_params
{
	rdo_png_params()
	{
		clear();
	}

	void clear()
	{
		m_orig_img.clear();
		m_output_file_data.clear();
		m_lambda = 300.0f;
		m_level = 0;
		m_psnr = 0;
		m_angular_rms_error = 0;
		m_y_psnr = 0;
		m_bpp = 0;
		m_print_debug_output = false;
		m_debug_images = false;
		m_print_progress = false;
		m_print_stats = false;

		m_use_chan_weights = false;
		m_chan_weights[0] = 1;
		m_chan_weights[1] = 1;
		m_chan_weights[2] = 1;
		m_chan_weights[3] = 1;

		{
			float LW = 2;
			float AW = 1.5;
			float BW = 1;
			float l = sqrtf(LW * LW + AW * AW + BW * BW);
			LW /= l;
			AW /= l;
			BW /= l;
			m_chan_weights_lab[0] = LW; // L
			m_chan_weights_lab[1] = AW; // a
			m_chan_weights_lab[2] = BW; // b
			m_chan_weights_lab[3] = 1.5f; // alpha
		}

		m_use_reject_thresholds = true;
		m_reject_thresholds[0] = 32;
		m_reject_thresholds[1] = 32;
		m_reject_thresholds[2] = 32;
		m_reject_thresholds[3] = 32;

		m_reject_thresholds_lab[0] = .05f;
		//m_reject_thresholds_lab[1] = .075f;
		m_reject_thresholds_lab[1] = .05f;

		m_transparent_reject_test = false;

		m_perceptual_error = true;

		m_match_only = false;

		m_two_pass = false;

		m_alpha_is_opacity = true;

		m_speed_mode = cFastestSpeed;

		m_max_smooth_std_dev = DEF_MAX_SMOOTH_STD_DEV;
		m_smooth_max_mse_scale = DEF_SMOOTH_MAX_MSE_SCALE;
		m_max_ultra_smooth_std_dev = DEF_MAX_ULTRA_SMOOTH_STD_DEV;
		m_ultra_smooth_max_mse_scale = DEF_ULTRA_SMOOTH_MAX_MSE_SCALE;

		m_no_mse_scaling = false;
	}

	void print()
	{
		printf("orig image: %ux%u has alpha: %u\n", m_orig_img.get_width(), m_orig_img.get_height(), m_orig_img.has_alpha());
		printf("lambda: %f\n", m_lambda);
		printf("level: %u\n", m_level);
		printf("chan weights: %u %u %u %u\n", m_chan_weights[0], m_chan_weights[1], m_chan_weights[2], m_chan_weights[3]);
		printf("use chan weights: %u\n", m_use_chan_weights);
		printf("chan weights lab: %f %f %f %f\n", m_chan_weights_lab[0], m_chan_weights_lab[1], m_chan_weights_lab[2], m_chan_weights_lab[3]);
		printf("reject thresholds: %u %u %u %u\n", m_reject_thresholds[0], m_reject_thresholds[1], m_reject_thresholds[2], m_reject_thresholds[3]);
		printf("reject thresholds lab: %f %f\n", m_reject_thresholds_lab[0], m_reject_thresholds_lab[1]);
		printf("use reject thresholds: %u\n", m_use_reject_thresholds);
		printf("transparent reject test: %u\n", m_transparent_reject_test);
		printf("print debug output: %u\n", m_print_debug_output);
		printf("debug images: %u\n", m_debug_images);
		printf("print progress: %u\n", m_print_progress);
		printf("print stats: %u\n", m_print_stats);
		printf("perceptual error: %u\n", m_perceptual_error);
		printf("match only: %u\n", m_match_only);
		printf("two pass: %u\n", m_two_pass);
		printf("alpha is opacity: %u\n", m_alpha_is_opacity);
		printf("speed mode: %u\n", (uint32_t)m_speed_mode);
		printf("max smooth std dev: %f\n", m_max_smooth_std_dev);
		printf("smooth max mse scale: %f\n", m_smooth_max_mse_scale);
		printf("max ultra smooth std dev: %f\n", m_max_ultra_smooth_std_dev);
		printf("ultra smooth max mse scale: %f\n", m_ultra_smooth_max_mse_scale);
		printf("no MSE scaling: %u\n", m_no_mse_scaling);
	}

	// TODO: results - move
	float m_psnr;
	float m_angular_rms_error;
	float m_y_psnr;
	float m_bpp;

	// This is the output image data, but note for PNG you can't save it at the right size without the scanline predictor values.
	image m_output_image;

	image m_orig_img;

	std::vector<uint8_t> m_output_file_data;

	float m_lambda;

	uint32_t m_level;

	uint32_t m_chan_weights[4];
	float m_chan_weights_lab[4];
	bool m_use_chan_weights;

	uint32_t m_reject_thresholds[4];
	float m_reject_thresholds_lab[2];
	bool m_use_reject_thresholds;

	bool m_transparent_reject_test;

	bool m_print_debug_output;
	bool m_debug_images;
	bool m_print_progress;
	bool m_print_stats;

	bool m_perceptual_error;

	bool m_match_only;
	bool m_two_pass;

	bool m_alpha_is_opacity;

	speed_mode m_speed_mode;

	float m_max_smooth_std_dev;
	float m_smooth_max_mse_scale;
	float m_max_ultra_smooth_std_dev;
	float m_ultra_smooth_max_mse_scale;

	bool m_no_mse_scaling;
};

static inline float square(float f)
{
	return f * f;
}

static inline uint32_t byteswap_32(uint32_t v)
{
	return ((v & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) | (((v >> 16) & 0xFF) << 8) | ((v >> 24) & 0xFF);
}

class tracked_stat
{
public:
	tracked_stat() { clear(); }

	inline void clear() { m_num = 0; m_total = 0; m_total2 = 0; }

	inline void update(uint32_t val) { m_num++; m_total += val; m_total2 += val * val; }

	inline tracked_stat& operator += (uint32_t val) { update(val); return *this; }

	inline uint32_t get_number_of_values() { return m_num; }
	inline uint64_t get_total() const { return m_total; }
	inline uint64_t get_total2() const { return m_total2; }

	inline float get_average() const { return m_num ? (float)m_total / m_num : 0.0f; };
	inline float get_std_dev() const { return m_num ? sqrtf((float)(m_num * m_total2 - m_total * m_total)) / m_num : 0.0f; }
	inline float get_variance() const { float s = get_std_dev(); return s * s; }

private:
	uint32_t m_num;
	uint64_t m_total;
	uint64_t m_total2;
};

struct Lab { float L; float a; float b; };
struct RGB { float r; float g; float b; };

static inline Lab linear_srgb_to_oklab(RGB c)
{
	float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
	float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
	float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

	float l_ = std::cbrtf(l);
	float m_ = std::cbrtf(m);
	float s_ = std::cbrtf(s);

	return
	{
		0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
		1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
		0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
	};
}

static float g_srgb_to_linear[256];

static float f_inv(float x)
{
	if (x <= 0.04045f)
		return x / 12.92f;
	else
		return powf(((x + 0.055f) / 1.055f), 2.4f);
}

static void init_srgb_to_linear()
{
	for (uint32_t i = 0; i < 256; i++)
		g_srgb_to_linear[i] = f_inv(i / 255.0f);
}

#pragma pack(push, 1)
struct Lab16
{
	uint16_t m_L, m_a, m_b;
};
#pragma pack(pop)

std::vector<Lab16> g_srgb_to_oklab16;

const float SCALE_L = 1.0f / 65535.0f;
const float SCALE_A = (1.0f / 65535.0f) * (0.276216f - (-0.233887f));
const float OFS_A = -0.233887f;
const float SCALE_B = (1.0f / 65535.0f) * (0.198570f - (-0.311528f));
const float OFS_B = -0.311528f;

const float MIN_L = 0.000000f, MAX_L = 1.000000f;
const float MIN_A = -0.233888f, MAX_A = 0.276217f;
const float MIN_B = -0.311529f, MAX_B = 0.198570f;

static inline Lab srgb_to_oklab(const color_rgba &c)
{
	const Lab16 &l = g_srgb_to_oklab16[c.r + c.g * 256 + c.b * 65536];

	Lab res;
	res.L = l.m_L * SCALE_L;
	res.a = l.m_a * SCALE_A + OFS_A;
	res.b = l.m_b * SCALE_B + OFS_B;

	return res;
}

static inline Lab srgb_to_oklab_norm(const color_rgba& c)
{
	const Lab16& l = g_srgb_to_oklab16[c.r + c.g * 256 + c.b * 65536];

	Lab res;
	res.L = l.m_L * SCALE_L;
	res.a = l.m_a * SCALE_L;
	res.b = l.m_b * SCALE_L;

	return res;
}

static void init_oklab_table(const char *pExec, bool quiet, bool caching_enabled)
{
	g_srgb_to_oklab16.resize(256 * 256 * 256);

	for (uint32_t r = 0; r <= 255; r++)
	{
		for (uint32_t g = 0; g <= 255; g++)
		{
			for (uint32_t b = 0; b <= 255; b++)
			{
				color_rgba c(r, g, b, 255);
				Lab l(linear_srgb_to_oklab({ g_srgb_to_linear[c.r], g_srgb_to_linear[c.g], g_srgb_to_linear[c.b] }));

				assert(l.L >= MIN_L && l.L <= MAX_L);
				assert(l.a >= MIN_A && l.a <= MAX_A);
				assert(l.b >= MIN_B && l.b <= MAX_B);

				float lL = std::round(((l.L - MIN_L) / (MAX_L - MIN_L)) * 65535.0f);
				float la = std::round(((l.a - MIN_A) / (MAX_A - MIN_A)) * 65535.0f);
				float lb = std::round(((l.b - MIN_B) / (MAX_B - MIN_B)) * 65535.0f);

				lL = clamp(lL, 0.0f, 65535.0f);
				la = clamp(la, 0.0f, 65535.0f);
				lb = clamp(lb, 0.0f, 65535.0f);

				Lab16& v = g_srgb_to_oklab16[r + g * 256 + b * 65536];
				v.m_L = (uint16_t)lL;
				v.m_a = (uint16_t)la;
				v.m_b = (uint16_t)lb;
			}
		}
	}
}

static inline float compute_se(const color_rgba& a, const color_rgba& orig, uint32_t num_comps, const rdo_png_params &params)
{
	float dist;

	if (params.m_perceptual_error)
	{
		Lab la = srgb_to_oklab_norm(a);
		Lab lb = srgb_to_oklab_norm(orig);

		la.L -= lb.L;
		la.a -= lb.a;
		la.b -= lb.b;

		float L_d = la.L * la.L;
		float a_d = la.a * la.a;
		float b_d = la.b * la.b;

		L_d *= params.m_chan_weights_lab[0];
		a_d *= params.m_chan_weights_lab[1];
		b_d *= params.m_chan_weights_lab[2];

		dist = L_d + a_d + b_d;

		// TODO: Scales the error to bring it into a range where lambda will be roughly comparable to plain MSE.
		const float NORM_ERROR_SCALE = 350000.0f;
		dist *= NORM_ERROR_SCALE;

		if (num_comps == 4)
		{
			int da = (int)a[3] - (int)orig[3];
			dist += params.m_chan_weights_lab[3] * square((float)da);
		}
	}
	else if (params.m_use_chan_weights)
	{
		int dr = (int)a[0] - (int)orig[0];
		int dg = (int)a[1] - (int)orig[1];
		int db = (int)a[2] - (int)orig[2];

		uint32_t idist = (uint32_t)(params.m_chan_weights[0] * (uint32_t)(dr * dr) + params.m_chan_weights[1] * (uint32_t)(dg * dg) + params.m_chan_weights[2] * (uint32_t)(db * db));
		if (num_comps == 4)
		{
			int da = (int)a[3] - (int)orig[3];
			idist += params.m_chan_weights[3] * (uint32_t)(da * da);
		}

		dist = (float)idist;
	}
	else
	{
		int dr = (int)a[0] - (int)orig[0];
		int dg = (int)a[1] - (int)orig[1];
		int db = (int)a[2] - (int)orig[2];

		uint32_t idist = (uint32_t)(dr * dr + dg * dg + db * db);
		if (num_comps == 4)
		{
			int da = (int)a[3] - (int)orig[3];
			idist += da * da;
		}

		dist = (float)idist;
	}

	return dist;
}

static inline bool should_reject(const color_rgba& trial_color, const color_rgba& orig_color, uint32_t num_comps, const rdo_png_params& params)
{
	if ((params.m_transparent_reject_test) && (num_comps == 4))
	{
		if ((orig_color[3] == 0) && (trial_color[3] > 0))
			return true;

		if ((orig_color[3] == 255) && (trial_color[3] < 255))
			return true;
	}

	if (params.m_use_reject_thresholds)
	{
		if (params.m_perceptual_error)
		{
			Lab t(srgb_to_oklab_norm(trial_color));
			Lab o(srgb_to_oklab_norm(orig_color));

			float L_diff = fabs(t.L - o.L);

			if (L_diff > params.m_reject_thresholds_lab[0])
				return true;

			float ab_dist = square(t.a - o.a) + square(t.b - o.b);

			if (ab_dist > (params.m_reject_thresholds_lab[1] * params.m_reject_thresholds_lab[1]))
				return true;

			if (num_comps == 4)
			{
				uint32_t delta_a = abs((int)trial_color[3] - (int)orig_color[3]);
				if (delta_a > params.m_reject_thresholds[3])
					return true;
			}
		}
		else
		{
			uint32_t delta_r = abs((int)trial_color[0] - (int)orig_color[0]);
			uint32_t delta_g = abs((int)trial_color[1] - (int)orig_color[1]);
			uint32_t delta_b = abs((int)trial_color[2] - (int)orig_color[2]);

			if (delta_r > params.m_reject_thresholds[0])
				return true;
			if (delta_g > params.m_reject_thresholds[1])
				return true;
			if (delta_b > params.m_reject_thresholds[2])
				return true;

			if (num_comps == 4)
			{
				uint32_t delta_a = abs((int)trial_color[3] - (int)orig_color[3]);
				if (delta_a > params.m_reject_thresholds[3])
					return true;
			}
		}
	}

	return false;
}

struct smooth_desc {
	bool alpha_is_opacity {true};
	float max_smooth_std_dev {DEF_MAX_SMOOTH_STD_DEV};
	float smooth_max_mse_scale {QOI_DEF_SMOOTH_MAX_MSE_SCALE};
	float max_ultra_smooth_std_dev {DEF_MAX_ULTRA_SMOOTH_STD_DEV};
	float ultra_smooth_max_mse_scale {QOI_DEF_ULTRA_SMOOTH_MAX_MSE_SCALE};
};

static void create_smooth_maps(
	vector2D<float> &smooth_block_mse_scales,
	const image& orig_img,
	const smooth_desc& desc
) {
	const uint32_t width = orig_img.get_width();
	const uint32_t height = orig_img.get_height();
	const uint32_t total_pixels = orig_img.get_total_pixels();
	const bool has_alpha = orig_img.has_alpha();
	const uint32_t num_comps = has_alpha ? 4 : 3;

#if 0
	if (params.m_no_mse_scaling)
	{
		smooth_block_mse_scales.set_all(1.0f);
		return;
	}
#endif

	image smooth_vis(width, height);
	image alpha_edge_vis(width, height);
	image ultra_smooth_vis(width, height);

	for (uint32_t y = 0; y < height; y++)
	{
		for (uint32_t x = 0; x < width; x++)
		{
			float alpha_edge_yl = 0.0f;
			if ((num_comps == 4) && (desc.alpha_is_opacity))
			{
				tracked_stat alpha_comp_stats;
				for (int yd = -3; yd <= 3; yd++)
				{
					for (int xd = -3; xd <= 3; xd++)
					{
						const color_rgba& p = orig_img.get_clamped((int)x + xd, (int)y + yd);
						alpha_comp_stats.update(p[3]);
					}
				}

				float max_std_dev = alpha_comp_stats.get_std_dev();

				float yl = clamp(max_std_dev / desc.max_smooth_std_dev, 0.0f, 1.0f);
				alpha_edge_yl = yl * yl;
			}

			{
				tracked_stat comp_stats[4];
				for (int yd = -1; yd <= 1; yd++)
				{
					for (int xd = -1; xd <= 1; xd++)
					{
						const color_rgba& p = orig_img.get_clamped((int)x + xd, (int)y + yd);
						comp_stats[0].update(p[0]);
						comp_stats[1].update(p[1]);
						comp_stats[2].update(p[2]);
						if (num_comps == 4)
							comp_stats[3].update(p[3]);
					}
				}

				float max_std_dev = 0.0f;
				for (uint32_t i = 0; i < num_comps; i++)
					max_std_dev = std::max(max_std_dev, comp_stats[i].get_std_dev());

				float yl = clamp(max_std_dev / desc.max_smooth_std_dev, 0.0f, 1.0f);
				yl = yl * yl;

				smooth_block_mse_scales(x, y) = lerp(desc.smooth_max_mse_scale, 1.0f, yl);

				if (num_comps == 4)
				{
					alpha_edge_vis(x, y).set((int)std::round(alpha_edge_yl * 255.0f));

					smooth_block_mse_scales(x, y) = lerp(smooth_block_mse_scales(x, y), desc.smooth_max_mse_scale, alpha_edge_yl);
				}

				smooth_vis(x, y).set(clamp((int)((smooth_block_mse_scales(x, y) - 1.0f) / (desc.smooth_max_mse_scale - 1.0f) * 255.0f + .5f), 0, 255));
			}

			{
				tracked_stat comp_stats[4];

				const int S = 5;
				for (int yd = -S; yd < S; yd++)
				{
					for (int xd = -S; xd < S; xd++)
					{
						const color_rgba& p = orig_img.get_clamped((int)x + xd, (int)y + yd);
						comp_stats[0].update(p[0]);
						comp_stats[1].update(p[1]);
						comp_stats[2].update(p[2]);
						if (num_comps == 4)
							comp_stats[3].update(p[3]);
					}
				}

				float max_std_dev = 0.0f;
				for (uint32_t i = 0; i < num_comps; i++)
					max_std_dev = std::max(max_std_dev, comp_stats[i].get_std_dev());

				float yl = clamp(max_std_dev / desc.max_ultra_smooth_std_dev, 0.0f, 1.0f);
				yl = powf(yl, 3.0f);

				smooth_block_mse_scales(x, y) = lerp(desc.ultra_smooth_max_mse_scale, smooth_block_mse_scales(x, y), yl);

				ultra_smooth_vis(x, y).set((int)std::round(yl * 255.0f));
			}

		}
	}

#if 0
	if (params.m_debug_images)
	{
		save_png("dbg_smooth_vis.png", smooth_vis);
		save_png("dbg_alpha_edge_vis.png", alpha_edge_vis);
		save_png("dbg_ultra_smooth_vis.png", ultra_smooth_vis);
	}
#endif
}

#pragma pack(push, 1)
struct qoi_header
{
	char magic[4]; // magic bytes "qoif"
	uint32_t width; // image width in pixels (BE)
	uint32_t height; // image height in pixels (BE)
	uint8_t channels; // 3 = RGB, 4 = RGBA
	uint8_t colorspace; // 0 = sRGB with linear alpha 1 = all channels linear
};
#pragma pack(pop)

static bool encode_rdo_qoi(
	const image& orig_img,
	std::vector<uint8_t>& data,
	//const rdo_png_params& params,
	const vector2D<float>& smooth_block_mse_scales,
	float lambda)
{
	// This function wasn't designed to deal with lambda=0, so nudge it up.
	lambda = max(lambda, .0000125f);

	const rdo_png_params params{};

	const bool has_alpha = orig_img.has_alpha();
	uint32_t num_comps = has_alpha ? 4 : 3;

	color_rgba hash[64];
	//clear_obj(hash);
	memset(&hash, 0, sizeof(hash));

	data.resize(0);

	qoi_header hdr;
	memcpy(hdr.magic, "qoif", 4);
	hdr.width = byteswap_32(orig_img.get_width());
	hdr.height = byteswap_32(orig_img.get_height());
	hdr.channels = has_alpha ? 4 : 3;
	hdr.colorspace = 0;
	data.resize(sizeof(hdr));
	memcpy(data.data(), &hdr, sizeof(hdr));

	int prev_r = 0, prev_g = 0, prev_b = 0, prev_a = 255;
	uint32_t cur_run_len = 0;

	enum commands_t
	{
		cRUN,
		cIDX,
		cDELTA,
		cLUMA,
		cRGB,
		cRGBA,
	};

	uint32_t total_run = 0, total_rgb = 0, total_rgba = 0, total_index = 0, total_delta = 0, total_luma = 0, total_run_pixels = 0;

	for (uint32_t y = 0; y < orig_img.get_height(); y++)
	{
		for (uint32_t x = 0; x < orig_img.get_width(); x++)
		{
			const color_rgba& c = orig_img(x, y);
			const float mse_scale = smooth_block_mse_scales(x, y);

			float best_mse = 0.0f;
			float best_bits = 40.0f;
			float best_t = best_mse + best_bits * lambda;
			int best_command = cRGBA;
			int best_index = 0, best_dr = 0, best_dg = 0, best_db = 0;

			{
				color_rgba trial_c(c.r, c.g, c.b, prev_a);
				if (!should_reject(trial_c, c, 4, params))
				{
					float mse = compute_se(trial_c, c, 4, params);
					float bits = 32.0f;
					float trial_t = mse_scale * mse + bits * lambda;
					if (trial_t < best_t)
					{
						best_mse = mse;
						best_bits = bits;
						best_t = trial_t;
						best_command = cRGB;
					}
				}
			}

			{
				color_rgba trial_c(prev_r, prev_g, prev_b, prev_a);
				if (!should_reject(trial_c, c, 4, params))
				{
					float mse = compute_se(trial_c, c, 4, params);
					float bits = cur_run_len ? 0 : 8.0f;
					float trial_t = mse_scale * mse + bits * lambda;
					if (trial_t < best_t)
					{
						best_mse = mse;
						best_bits = bits;
						best_t = trial_t;
						best_command = cRUN;

						if (best_mse == 0.0f)
						{
							cur_run_len++;
							if (cur_run_len == 62)
							{
								total_run_pixels += cur_run_len;

								data.push_back(0xC0 | (cur_run_len - 1));
								cur_run_len = 0;

								total_run++;
							}

							hash[(prev_r * 3 + prev_g * 5 + prev_b * 7 + prev_a * 11) & 63].set(prev_r, prev_g, prev_b, prev_a);

							continue;
						}
					}
				}
			}

			if (8.0f * lambda < best_t)
			{
				uint32_t hash_idx = (c.r * 3 + c.g * 5 + c.b * 7 + c.a * 11) & 63;

				// First try the INDEX command losslessly.
				if (c == hash[hash_idx])
				{
					float bits = 8.0f;
					float trial_t = bits * lambda;

					assert(trial_t < best_t);

					best_mse = 0.0f;
					best_bits = bits;
					best_t = trial_t;
					best_command = cIDX;
					best_index = hash_idx;
				}
				else
				{
					// Try a lossy INDEX command.
					for (uint32_t i = 0; i < 64; i++)
					{
						if (!should_reject(hash[i], c, 4, params))
						{
							float mse = compute_se(hash[i], c, 4, params);
							float bits = 8.0f;
							float trial_t = mse_scale * mse + bits * lambda;
							if (trial_t < best_t)
							{
								best_mse = mse;
								best_bits = bits;
								best_t = trial_t;
								best_command = cIDX;
								best_index = i;
							}
						}
					}
				}
			}

			if (8.0f * lambda < best_t)
			{
				bool delta_encodable_losslessly = false;

				// First try the DELTA command losslessly.
				if (c.a == prev_a)
				{
					int dr = ((int)c.r - prev_r + 2) & 255;
					int dg = ((int)c.g - prev_g + 2) & 255;
					int db = ((int)c.b - prev_b + 2) & 255;

					if ((dr <= 3) && (dg <= 3) && (db <= 3))
					{
						delta_encodable_losslessly = true;

						float bits = 8.0f;
						float trial_t = bits * lambda;

						assert(trial_t < best_t);

						best_mse = 0.0f;
						best_bits = bits;
						best_t = trial_t;
						best_command = cDELTA;
						best_dr = dr - 2;
						best_dg = dg - 2;
						best_db = db - 2;
					}
				}

				// Try a lossy DELTA command.
				if (!delta_encodable_losslessly)
				{
					for (uint32_t i = 0; i < 64; i++)
					{
						int dr = ((i >> 4) & 3) - 2;
						int dg = ((i >> 2) & 3) - 2;
						int db = (i & 3) - 2;

						color_rgba trial_c((prev_r + dr) & 255, (prev_g + dg) & 255, (prev_b + db) & 255, prev_a);

						if (!should_reject(trial_c, c, 4, params))
						{
							float mse = compute_se(trial_c, c, 4, params);
							float bits = 8.0f;
							float trial_t = mse_scale * mse + bits * lambda;

							if (trial_t < best_t)
							{
								best_mse = mse;
								best_bits = bits;
								best_t = trial_t;
								best_command = cDELTA;
								best_dr = dr;
								best_dg = dg;
								best_db = db;
							}
						}
					}
				}
			}

			if (16.0f * lambda < best_t)
			{
				bool luma_encodable_losslessly_in_rgb = false;

				// First try the LUMA command losslessly in RGB (may not be lossy in alpha).
				{
					int g_diff = (int)c.g - prev_g;

					int dg = (g_diff + 32) & 255;

					int dr = (((int)c.r - prev_r) - g_diff + 8) & 255;
					int db = (((int)c.b - prev_b) - g_diff + 8) & 255;

					if ((dg <= 63) && (dr <= 15) && (db <= 15))
					{
						luma_encodable_losslessly_in_rgb = true;

						color_rgba trial_c(c.r, c.g, c.b, prev_a);

						if (!should_reject(trial_c, c, 4, params))
						{
							float mse = compute_se(trial_c, c, 4, params);
							float bits = 16.0f;
							float trial_t = mse_scale * mse + bits * lambda;

							if (trial_t < best_t)
							{
								best_mse = mse;
								best_bits = bits;
								best_t = trial_t;
								best_command = cLUMA;
								best_dr = dr - 8;
								best_dg = dg - 32;
								best_db = db - 8;
							}
						}
					}
				}

				// If we can't use it losslessly, try it lossy.
				if ((!luma_encodable_losslessly_in_rgb) && (params.m_speed_mode != cFastestSpeed))
				{
					if (params.m_speed_mode == cNormalSpeed)
					{
						// Search all encodable LUMA commands.
						for (uint32_t i = 0; i < 16384; i++)
						{
							int dr = ((i >> 6) & 15) - 8;
							int dg = (i & 63) - 32;
							int db = ((i >> 10) & 15) - 8;

							color_rgba trial_c((prev_r + dg + dr) & 255, (prev_g + dg) & 255, (prev_b + dg + db) & 255, prev_a);

							if (!should_reject(trial_c, c, 4, params))
							{
								float mse = compute_se(trial_c, c, 4, params);
								float bits = 16.0f;
								float trial_t = mse_scale * mse + bits * lambda;

								if (trial_t < best_t)
								{
									best_mse = mse;
									best_bits = bits;
									best_t = trial_t;
									best_command = cLUMA;
									best_dr = dr;
									best_dg = dg;
									best_db = db;
								}
							}
						}
					}
					else
					{
						// TODO: This isn't very smart. What if the G delta is encodable but R and/or B aren't?
						const int g_deltas[] = { -24, -16, -14, -12, -10, -8, -6, -4, -3, -2, -1, 0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 24 };
						const int TOTAL_G_DELTAS = sizeof(g_deltas) / sizeof(g_deltas[0]);

						for (int kg = 0; kg < TOTAL_G_DELTAS; kg++)
						{
							const int dg = g_deltas[kg];
							for (uint32_t i = 0; i < 256; i++)
							{
								int dr = (i & 15) - 8;
								int db = ((i >> 4) & 15) - 8;

								color_rgba trial_c((prev_r + dg + dr) & 255, (prev_g + dg) & 255, (prev_b + dg + db) & 255, prev_a);

								if (!should_reject(trial_c, c, 4, params))
								{
									float mse = compute_se(trial_c, c, 4, params);
									float bits = 16.0f;
									float trial_t = mse_scale * mse + bits * lambda;

									if (trial_t < best_t)
									{
										best_mse = mse;
										best_bits = bits;
										best_t = trial_t;
										best_command = cLUMA;
										best_dr = dr;
										best_dg = dg;
										best_db = db;
									}
								}
							}
						}
					}
				}
			}

			switch (best_command)
			{
			case cRUN:
			{
				cur_run_len++;
				if (cur_run_len == 62)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				hash[(prev_r * 3 + prev_g * 5 + prev_b * 7 + prev_a * 11) & 63].set(prev_r, prev_g, prev_b, prev_a);

				break;
			}
			case cRGB:
			{
				if (cur_run_len)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				data.push_back(254);
				data.push_back((uint8_t)c.r);
				data.push_back((uint8_t)c.g);
				data.push_back((uint8_t)c.b);
				hash[(c.r * 3 + c.g * 5 + c.b * 7 + prev_a * 11) & 63].set(c.r, c.g, c.b, prev_a);
				prev_r = c.r;
				prev_g = c.g;
				prev_b = c.b;

				total_rgb++;

				break;
			}
			case cRGBA:
			{
				if (cur_run_len)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				data.push_back(255);
				data.push_back((uint8_t)c.r);
				data.push_back((uint8_t)c.g);
				data.push_back((uint8_t)c.b);
				data.push_back((uint8_t)c.a);
				hash[(c.r * 3 + c.g * 5 + c.b * 7 + c.a * 11) & 63] = c;
				prev_r = c.r;
				prev_g = c.g;
				prev_b = c.b;
				prev_a = c.a;

				total_rgba++;

				break;
			}
			case cIDX:
			{
				if (cur_run_len)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				data.push_back(best_index);

				prev_r = hash[best_index].r;
				prev_g = hash[best_index].g;
				prev_b = hash[best_index].b;
				prev_a = hash[best_index].a;

				total_index++;

				break;
			}
			case cDELTA:
			{
				if (cur_run_len)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				assert(best_dr >= -2 && best_dr <= 1);
				assert(best_dg >= -2 && best_dg <= 1);
				assert(best_db >= -2 && best_db <= 1);

				data.push_back(64 + ((best_dr + 2) << 4) + ((best_dg + 2) << 2) + (best_db + 2));

				uint32_t decoded_r = (prev_r + best_dr) & 0xFF;
				uint32_t decoded_g = (prev_g + best_dg) & 0xFF;
				uint32_t decoded_b = (prev_b + best_db) & 0xFF;
				uint32_t decoded_a = prev_a;

				hash[(decoded_r * 3 + decoded_g * 5 + decoded_b * 7 + decoded_a * 11) & 63].set(decoded_r, decoded_g, decoded_b, decoded_a);

				prev_r = decoded_r;
				prev_g = decoded_g;
				prev_b = decoded_b;
				prev_a = decoded_a;

				total_delta++;

				break;
			}
			case cLUMA:
			{
				if (cur_run_len)
				{
					total_run_pixels += cur_run_len;

					data.push_back(0xC0 | (cur_run_len - 1));
					cur_run_len = 0;

					total_run++;
				}

				assert(best_dr >= -8 && best_dr <= 7);
				assert(best_dg >= -32 && best_dg <= 31);
				assert(best_db >= -8 && best_db <= 7);

				data.push_back((uint8_t)(128 + (best_dg + 32)));
				data.push_back((uint8_t)(((best_dr + 8) << 4) | (best_db + 8)));

				uint32_t decoded_r = (prev_r + best_dr + best_dg) & 0xFF;
				uint32_t decoded_g = (prev_g + best_dg) & 0xFF;
				uint32_t decoded_b = (prev_b + best_db + best_dg) & 0xFF;
				uint32_t decoded_a = prev_a;

				hash[(decoded_r * 3 + decoded_g * 5 + decoded_b * 7 + decoded_a * 11) & 63].set(decoded_r, decoded_g, decoded_b, decoded_a);

				prev_r = decoded_r;
				prev_g = decoded_g;
				prev_b = decoded_b;
				prev_a = decoded_a;

				total_luma++;

				break;
			}
			default:
			{
				assert(0);
				break;
			}
			}

		}
	}

	if (cur_run_len)
	{
		total_run_pixels += cur_run_len;

		data.push_back((64 + 128) | (cur_run_len - 1));
		cur_run_len = 0;

		total_run++;
	}

	// end padding
	for (uint32_t i = 0; i < 7; i++) {
		data.push_back(0);
	}
	data.push_back(1);

	if (params.m_print_stats)
	{
		printf("Totals: Run: %u, Run Pixels: %u %3.2f%%, RGB: %u %3.2f%%, RGBA: %u %3.2f%%, INDEX: %u %3.2f%%, DELTA: %u %3.2f%%, LUMA: %u %3.2f%%\n\n",
			total_run,
			total_run_pixels, (total_run_pixels * 100.0f) / orig_img.get_total_pixels(),
			total_rgb, (total_rgb * 100.0f) / orig_img.get_total_pixels(),
			total_rgba, (total_rgba * 100.0f) / orig_img.get_total_pixels(),
			total_index, (total_index * 100.0f) / orig_img.get_total_pixels(),
			total_delta, (total_delta * 100.0f) / orig_img.get_total_pixels(),
			total_luma, (total_luma * 100.0f) / orig_img.get_total_pixels());
	}

	return true;
}

static bool g_init {false};

bool init_qoi_rdo(void) {
	if (g_init) {
		return false;
	}
	init_srgb_to_linear();
	init_oklab_table("", true, false);
	g_init = true;
	return true;
}

bool quit_qoi_rdo(void) {
	if (!g_init) {
		return false;
	}
	g_srgb_to_oklab16.clear();

	return true;
}

static float lambda_from_quality(int quality) {
	quality = clamp(quality, 1, 100);

	// TODO: more stuff and log scale
	//return lerp(50000, 100, quality/100.f);
	//return lerp(250'000, 0, sqrtf(quality/100.f));
	//return lerp(1'000'000, 0, sqrtf(quality/100.f));
	//return lerp(1'000'000, 0, clamp(log10f(quality/100.f)+1, 0.f, 1.f));
	//return lerp(250'000, 0, clamp(log10f(quality/100.f)+1, 0.f, 1.f));
	return lerp(250'000, 0, cbrtf(quality/100.f));
}

std::vector<uint8_t> encode_qoi_rdo_simple(const uint8_t* data, const qoi_rdo_desc& desc, int quality) {
	if (!g_init) {
		return {};
	}

	const float lambda = lambda_from_quality(quality);

	vector2D<float> smooth_block_mse_scales(desc.width, desc.height);

	image orig_img(data, desc.width, desc.height, desc.channels);

	if (false /* m_no_mse_scaling */) {
		smooth_block_mse_scales.set_all(1.0f);
	} else {
		create_smooth_maps(
			smooth_block_mse_scales,
			orig_img,
			{} // smooth_desc
		);
	}

	std::vector<uint8_t> output_data;

	if (!encode_rdo_qoi(
		orig_img,
		output_data,
		smooth_block_mse_scales,
		lambda))
	{
		return {};
	}

	return output_data;
}

