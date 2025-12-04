#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

class MessageImageLoader {
	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		MessageImageLoader(void);
		TextureLoaderResult load(TextureUploaderI& tu, Message3Handle m, uint32_t w, uint32_t h);
};

