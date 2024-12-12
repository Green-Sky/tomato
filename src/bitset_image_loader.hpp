#pragma once

#include <solanaceae/object_store/fwd.hpp>

#include "./texture_cache.hpp"

#include <optional>

class BitsetImageLoader {
	public:
		BitsetImageLoader(void);
		std::optional<TextureEntry> load(TextureUploaderI& tu, ObjectHandle o);
};

