#include "./tox_avatar_loader.hpp"

#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_qoi.hpp"
#include "./image_loader_stb.hpp"
#include "./image_loader_webp.hpp"
#include "./image_loader_sdl_image.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/tox_contacts/components.hpp>

#include <sodium/crypto_hash_sha256.h>

#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>

// fwd
namespace Message {
uint64_t getTimeMS(void);
}

ToxAvatarLoader::ToxAvatarLoader(Contact3Registry& cr) : _cr(cr) {
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderQOI>());
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLImage>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSTB>());
}

static float getHue_6bytes(const uint8_t* data) {
	uint64_t hue_uint = 0x00;
	for (size_t i = 0; i < 6; i++) {
		hue_uint = hue_uint << 8;
		hue_uint += data[i];
	}

	// 48bit max (6bytes)
	return float(hue_uint / 281474976710655.);
}

static float hue2Rgb(float p, float q, float t) {
	while (t < 0.f) { t += 1.f; }
	while (t > 1.f) { t -= 1.f; }
	if (t < 1.f/6.f) { return p + (q - p) * 6.f * t; }
	if (t < 1.f/2.f) { return q; }
	if (t < 2.f/3.f) { return p + (q - p) * (4.f - 6.f * t); }
	return p;
}

//https://github.com/Tox/Tox-Client-Standard/blob/master/appendix/ToxIdenticons.md
// creates a 5x5 pix texture RGBA8888
static std::vector<uint8_t> generateToxIdenticon(const ToxKey& key) {
	// first hash key
	std::array<uint8_t, 32> hashed_key;
	assert(hashed_key.size() == key.data.size());
	crypto_hash_sha256(hashed_key.data(), key.data.data(), key.data.size());

	const float hue_color1 = getHue_6bytes(hashed_key.data()+26);
	const float hue_color2 = getHue_6bytes(hashed_key.data()+20);

	const float sat_color1 = 0.5f;
	const float lig_color1 = 0.3f;

	const float sat_color2 = 0.5f;
	const float lig_color2 = 0.8f;

	// TODO: refactor this mess

#define Q(LIG, SAT) (LIG) < 0.5f ? (LIG) * (1 + (SAT)) : (LIG) + (SAT) - (LIG) * (SAT)
	const float q_color1 = Q(lig_color1, sat_color1);
	const float q_color2 = Q(lig_color2, sat_color2);
#undef Q

#define P(LIG, Q) 2.f * (LIG) - (Q)
	const float p_color1 = P(lig_color1, q_color1);
	const float p_color2 = P(lig_color2, q_color2);
#undef P

	const uint8_t color_1_r = hue2Rgb(p_color1, q_color1, hue_color1 + 1.f/3.f) * 255.f + 0.499f;
	const uint8_t color_1_g = hue2Rgb(p_color1, q_color1, hue_color1) * 255.f + 0.499f;
	const uint8_t color_1_b = hue2Rgb(p_color1, q_color1, hue_color1 - 1.f/3.f) * 255.f + 0.499f;

	const uint8_t color_2_r = hue2Rgb(p_color2, q_color2, hue_color2 + 1.f/3.f) * 255.f + 0.499f;
	const uint8_t color_2_g = hue2Rgb(p_color2, q_color2, hue_color2) * 255.f + 0.499f;
	const uint8_t color_2_b = hue2Rgb(p_color2, q_color2, hue_color2 - 1.f/3.f) * 255.f + 0.499f;

	// start drawing

	std::vector<uint8_t> pixels(5*5*4, 0xff); // fill with white
#define R(x,y) pixels[(y*4*5) + (4*x+0)]
#define G(x,y) pixels[(y*4*5) + (4*x+1)]
#define B(x,y) pixels[(y*4*5) + (4*x+2)]
#define COLOR1(x,y) R(x, y) = color_1_r; G(x, y) = color_1_g; B(x, y) = color_1_b;
#define COLOR2(x,y) R(x, y) = color_2_r; G(x, y) = color_2_g; B(x, y) = color_2_b;

	for (int64_t y = 0; y < 5; y++) {
		for (int64_t x = 0; x < 5; x++) {
			// mirrored index
			//int64_t m_x = x <= 3 ? 3 : x - (x-2)*2;
			int64_t m_x = ((x*2)-4)/2;
			m_x = m_x < 0 ? -m_x : m_x;

			int64_t pos = y * 3 + m_x;
			const uint8_t byte = hashed_key[pos];
			const uint8_t color = byte % 2;
			if (color == 0) {
				COLOR1(x, y)
			} else {
				COLOR2(x, y)
			}
		}
	}

#undef COLOR1
#undef COLOR2
#undef R
#undef G
#undef B
	return pixels;
}

std::optional<TextureEntry> ToxAvatarLoader::load(TextureUploaderI& tu, Contact3 c) {
	if (!_cr.valid(c)) {
		return std::nullopt;
	}

	if (_cr.all_of<Contact::Components::AvatarMemory>(c)) {
		const auto& a_m = _cr.get<Contact::Components::AvatarMemory>(c);

		TextureEntry new_entry;
		new_entry.timestamp_last_rendered = Message::getTimeMS();
		new_entry.current_texture = 0;

		new_entry.width = a_m.width;
		new_entry.height = a_m.height;

		const auto n_t = tu.uploadRGBA(a_m.data.data(), a_m.width, a_m.height);
		new_entry.textures.push_back(n_t);
		new_entry.frame_duration.push_back(250);

		std::cout << "TAL: loaded memory buffer\n";

		return new_entry;
	}

	if (_cr.all_of<Contact::Components::AvatarFile>(c)) {
		const auto& a_f = _cr.get<Contact::Components::AvatarFile>(c);

		std::ifstream file(a_f.file_path, std::ios::binary);
		if (file.is_open()) {
			std::vector<uint8_t> tmp_buffer;
			while (file.good()) {
				auto ch = file.get();
				if (ch == EOF) {
					break;
				} else {
					tmp_buffer.push_back(ch);
				}
			}

			// try all loaders after another
			for (auto& il : _image_loaders) {
				auto res = il->loadFromMemoryRGBA(tmp_buffer.data(), tmp_buffer.size());
				if (res.frames.empty() || res.height == 0 || res.width == 0) {
					continue;
				}

				TextureEntry new_entry;
				new_entry.timestamp_last_rendered = Message::getTimeMS();
				new_entry.current_texture = 0;
				for (const auto& [ms, data] : res.frames) {
					const auto n_t = tu.uploadRGBA(data.data(), res.width, res.height);
					new_entry.textures.push_back(n_t);
					new_entry.frame_duration.push_back(ms);
				}

				new_entry.width = res.width;
				new_entry.height = res.height;

				std::cout << "TAL: loaded image file " << a_f.file_path << "\n";

				return new_entry;
			}
		}
	} // continues if loading img fails

	if (!_cr.any_of<
		Contact::Components::ToxFriendPersistent,
		Contact::Components::ToxGroupPersistent,
		Contact::Components::ToxGroupPeerPersistent
	>(c)) {
		return std::nullopt;
	}

	std::vector<uint8_t> pixels;
	if (_cr.all_of<Contact::Components::ToxFriendPersistent>(c)) {
		pixels = generateToxIdenticon(_cr.get<Contact::Components::ToxFriendPersistent>(c).key);
	} else if (_cr.all_of<Contact::Components::ToxGroupPersistent>(c)) {
		pixels = generateToxIdenticon(_cr.get<Contact::Components::ToxGroupPersistent>(c).chat_id);
	} else if (_cr.all_of<Contact::Components::ToxGroupPeerPersistent>(c)) {
		pixels = generateToxIdenticon(_cr.get<Contact::Components::ToxGroupPeerPersistent>(c).peer_key);
	}

	TextureEntry new_entry;
	new_entry.timestamp_last_rendered = Message::getTimeMS();
	new_entry.current_texture = 0;

	const auto n_t = tu.uploadRGBA(pixels.data(), 5, 5, TextureUploaderI::NEAREST);
	new_entry.textures.push_back(n_t);
	new_entry.frame_duration.push_back(250);

	new_entry.width = 5;
	new_entry.height = 5;

	std::cout << "TAL: generated ToxIdenticon\n";

	return new_entry;
}

