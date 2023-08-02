#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./image_loader.hpp"
#include "./texture_cache.hpp"

#include <optional>

class MessageImageLoader {
	std::vector<std::unique_ptr<ImageLoaderI>> _image_loaders;

	public:
		MessageImageLoader(void);
		std::optional<TextureEntry> load(TextureUploaderI& tu, Message3Handle c);
};

// TODO: move to rmm
template<>
struct std::hash<Message3Handle> {
	std::size_t operator()(Message3Handle const& m) const noexcept {
		const std::size_t h1 = reinterpret_cast<std::size_t>(m.registry());
		const std::size_t h2 = entt::to_integral(m.entity());
		return (h1 << 3) ^ (h2 * 11400714819323198485llu);
	}
};

