#pragma once

#include <solanaceae/contact/contact_model3.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

#include <optional>

class ToxAvatarLoader {
	Contact3Registry& _cr;

	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		ToxAvatarLoader(Contact3Registry& cr);
		TextureLoaderResult load(TextureUploaderI& tu, Contact3 c);
};

