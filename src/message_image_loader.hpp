#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

#include <optional>

class MessageImageLoader {
	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		MessageImageLoader(void);
		std::optional<TextureEntry> load(TextureUploaderI& tu, Message3Handle m);
};

