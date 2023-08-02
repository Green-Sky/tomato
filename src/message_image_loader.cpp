#include "./message_image_loader.hpp"

#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_stb.hpp"
#include "./image_loader_webp.hpp"

#include <solanaceae/message3/components.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>

MessageImageLoader::MessageImageLoader(void) {
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSTB>());
}

std::optional<TextureEntry> MessageImageLoader::load(TextureUploaderI& tu, Message3Handle m) {
	if (!static_cast<bool>(m)) {
		return std::nullopt;
	}

	if (m.all_of<Message::Components::Transfer::FileInfoLocal>()) {
		const auto& file_list = m.get<Message::Components::Transfer::FileInfoLocal>().file_list;
		assert(!file_list.empty());
		const auto& file_path = file_list.front();

		std::ifstream file(file_path, std::ios::binary);
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
				new_entry.timestamp_last_rendered = getNowMS();
				new_entry.current_texture = 0;
				for (const auto& [ms, data] : res.frames) {
					const auto n_t = tu.uploadRGBA(data.data(), res.width, res.height);
					new_entry.textures.push_back(n_t);
					new_entry.frame_duration.push_back(ms);
				}

				new_entry.width = res.width;
				new_entry.height = res.height;

				std::cout << "MIL: loaded image file " << file_path << "\n";

				return new_entry;
			}
		}
	}

	return std::nullopt;
}

