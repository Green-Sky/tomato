#pragma once

#include <solanaceae/contact/fwd.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

#include <optional>

class ToxAvatarLoader {
	ContactStore4I& _cs;

	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		ToxAvatarLoader(ContactStore4I& cr);
		TextureLoaderResult load(TextureUploaderI& tu, Contact4 c);
};

