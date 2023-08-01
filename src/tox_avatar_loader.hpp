#pragma once

#include <solanaceae/contact/contact_model3.hpp>

#include "./texture_cache.hpp"

#include <optional>

class ToxAvatarLoader {
	Contact3Registry& _cr;

	public:
		ToxAvatarLoader(Contact3Registry& cr) : _cr(cr) {}
		std::optional<TextureEntry> load(TextureUploaderI& tu, Contact3 c);
};

