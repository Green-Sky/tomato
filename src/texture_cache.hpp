#pragma once

#include "./texture_uploader.hpp"

#include <entt/container/dense_map.hpp>
#include <entt/container/dense_set.hpp>

#include <iostream>

struct TextureEntry {
	uint32_t width {0};
	uint32_t height {0};
	std::vector<uint64_t> textures;
	std::vector<uint32_t> frame_duration; // ms
	size_t current_texture {0};

	bool rendered_this_frame {false};
	// or flipped for animations
	int64_t timestamp_last_rendered {0}; // ms

	TextureEntry(void) = default;
	TextureEntry(const TextureEntry& other) :
		width(other.width),
		height(other.height),
		textures(other.textures),
		frame_duration(other.frame_duration),
		current_texture(other.current_texture),

		rendered_this_frame(other.rendered_this_frame),
		timestamp_last_rendered(other.timestamp_last_rendered)
	{}

	TextureEntry& operator=(const TextureEntry& other) {
		width = other.width;
		height = other.height;
		textures = other.textures;
		frame_duration = other.frame_duration;
		current_texture = other.current_texture;

		rendered_this_frame = other.rendered_this_frame;
		timestamp_last_rendered = other.timestamp_last_rendered;
		return *this;
	}

	uint32_t getDuration(void) const {
		return frame_duration.at(current_texture);
	}

	void next(void) {
		current_texture = (current_texture + 1) % frame_duration.size();
	}

	void doAnimation(const int64_t ts_now);

	template<typename TextureType>
	TextureType getID(void) {
		static_assert(
			sizeof(TextureType) == sizeof(uint64_t) ||
			sizeof(TextureType) == sizeof(uint32_t)
		);

		rendered_this_frame = true;
		assert(current_texture < textures.size());
		if constexpr (sizeof(TextureType) == sizeof(uint64_t)) {
			return reinterpret_cast<TextureType>(textures.at(current_texture));
		} else if constexpr (sizeof(TextureType) == sizeof(uint32_t)) {
			return reinterpret_cast<TextureType>(static_cast<uint32_t>(textures.at(current_texture)));
		}
	}
};

TextureEntry generateTestAnim(TextureUploaderI& tu);

// TODO: move to utils or something
uint64_t getNowMS(void);

template<typename TextureType, typename KeyType, class Loader>
struct TextureCache {
	static_assert(
		sizeof(TextureType) == sizeof(uint64_t) ||
		sizeof(TextureType) == sizeof(uint32_t)
	);

	Loader& _l;
	TextureUploaderI& _tu;

	TextureEntry _default_texture;

	entt::dense_map<KeyType, TextureEntry> _cache;
	entt::dense_set<KeyType> _to_load;

	const uint64_t ms_before_purge {60 * 1000ull};
	const size_t min_count_before_purge {0}; // starts purging after that

	TextureCache(Loader& l, TextureUploaderI& tu) : _l(l), _tu(tu) {
		//_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
		//_image_loaders.push_back(std::make_unique<ImageLoaderSTB>());
		_default_texture = generateTestAnim(_tu);
		//_default_texture = loadTestWebPAnim();
	}

	~TextureCache(void) {
		for (const auto& it : _cache) {
			for (const auto& tex_id : it.second.textures) {
				_tu.destroy(tex_id);
			}
		}
	}

	struct GetInfo {
		TextureType id;
		uint32_t width;
		uint32_t height;
	};
	GetInfo get(const KeyType& key) {
		auto it = _cache.find(key);

		if (it != _cache.end()) {
			return {
				it->second.template getID<TextureType>(),
				it->second.width,
				it->second.height
			};
		} else {
			_to_load.insert(key);
			return {
				_default_texture.getID<TextureType>(),
				_default_texture.width,
				_default_texture.height
			};
		}
	}

	void update(void) {
		const uint64_t ts_now = getNowMS();

		std::vector<KeyType> to_purge;
		for (auto&& [key, te] : _cache) {
			if (te.rendered_this_frame) {
				te.doAnimation(ts_now);
				te.rendered_this_frame = false;
			} else if (_cache.size() > min_count_before_purge && ts_now - te.timestamp_last_rendered >= ms_before_purge) {
				to_purge.push_back(key);
			}
		}

		invalidate(to_purge);

		_default_texture.doAnimation(ts_now);
	}

	void invalidate(const std::vector<KeyType>& to_purge) {
		for (const auto& key : to_purge) {
			for (const auto& tex_id : _cache.at(key).textures) {
				_tu.destroy(tex_id);
			}
			_cache.erase(key);
		}
	}

	void workLoadQueue(void) {
		for (auto it = _to_load.begin(); it != _to_load.end(); it++) {
			auto new_entry_opt = _l.load(_tu, *it);
			if (new_entry_opt.has_value()) {
				_cache.emplace(*it, new_entry_opt.value());
				_to_load.erase(it);
				// TODO: not a good idea
				break; // end load from queue/onlyload 1 per update
			}
		}
	}
};

