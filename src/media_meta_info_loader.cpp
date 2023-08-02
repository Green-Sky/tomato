#include "./media_meta_info_loader.hpp"

#include "./image_loader_webp.hpp"
#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_stb.hpp"

#include <solanaceae/message3/components.hpp>

#include <fstream>
#include <iostream>

MediaMetaInfoLoader::MediaMetaInfoLoader(RegistryMessageModel& rmm) : _rmm(rmm) {
	// HACK: make them be added externally?
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSTB>());

	_rmm.subscribe(this, RegistryMessageModel_Event::message_construct);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_updated);
}

MediaMetaInfoLoader::~MediaMetaInfoLoader(void) {
}

bool MediaMetaInfoLoader::onEvent(const Message::Events::MessageConstruct& e) {
	return false;
}

bool MediaMetaInfoLoader::onEvent(const Message::Events::MessageUpdated& e) {
	if (e.e.any_of<Message::Components::TagNotImage, Message::Components::FrameDims>()) {
		return false;
	}

	if (!e.e.all_of<Message::Components::Transfer::FileInfoLocal, Message::Components::Transfer::TagHaveAll>()) {
		return false;
	}

	const auto& fil = e.e.get<Message::Components::Transfer::FileInfoLocal>();
	if (fil.file_list.size() != 1) {
		return false;
	}

	std::ifstream file(fil.file_list.front(), std::ios::binary);
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

		bool could_load {false};
		// try all loaders after another
		for (auto& il : _image_loaders) {
			auto res = il->loadInfoFromMemory(tmp_buffer.data(), tmp_buffer.size());
			if (res.height == 0 || res.width == 0) {
				continue;
			}

			e.e.emplace<Message::Components::FrameDims>(res.width, res.height);

			could_load = true;

			std::cout << "MMIL loaded image info " << fil.file_list.front() << "\n";

			_rmm.throwEventUpdate(e.e);
			break;
		}

		if (!could_load) {
			e.e.emplace<Message::Components::TagNotImage>();

			std::cout << "MMIL loading failed image info " << fil.file_list.front() << "\n";

			_rmm.throwEventUpdate(e.e);
		}
	}

	return false;
}

