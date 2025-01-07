#include "./message_image_loader.hpp"

#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_qoi.hpp"
#include "./image_loader_webp.hpp"
#include "./image_loader_sdl_image.hpp"
#include "./media_meta_info_loader.hpp"

#include <solanaceae/message3/components.hpp>

#include "./os_comps.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <solanaceae/file/file2.hpp>

#include <entt/entity/entity.hpp>

#include <iostream>

MessageImageLoader::MessageImageLoader(void) {
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderQOI>());
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLImage>());
}

TextureLoaderResult MessageImageLoader::load(TextureUploaderI& tu, Message3Handle m) {
	if (!static_cast<bool>(m)) {
		return {std::nullopt};
	}

	if (m.all_of<Message::Components::TagNotImage>()) {
		return {std::nullopt};
	}

	if (!m.all_of<Message::Components::MessageFileObject>()) {
		// not a file message
		return {std::nullopt};
	}
	const auto& o = m.get<Message::Components::MessageFileObject>().o;

	if (!static_cast<bool>(o)) {
		std::cerr << "MIL error: invalid object in file message\n";
		return {std::nullopt};
	}

	if (!o.all_of<ObjComp::Ephemeral::Backend, ObjComp::F::SingleInfo>()) {
		std::cerr << "MIL error: object missing backend (?)\n";
		return {std::nullopt};
	}

	// TODO: handle collections
	const auto file_size = o.get<ObjComp::F::SingleInfo>().file_size;

	if (file_size > 50*1024*1024) {
		std::cerr << "MIL error: image file too large\n";
		return {std::nullopt};
	}

	if (file_size == 0) {
		std::cerr << "MIL warning: empty file\n";
		return {std::nullopt};
	}

	if (!o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		// not ready yet
		return {std::nullopt};
	}

	auto* file_backend = o.get<ObjComp::Ephemeral::Backend>().ptr;
	if (file_backend == nullptr) {
		std::cerr << "MIL error: object backend nullptr\n";
		return {std::nullopt};
	}

	auto file2 = file_backend->file2(o, StorageBackendI::FILE2_READ);
	if (!file2 || !file2->isGood() || !file2->can_read) {
		std::cerr << "MIL error: creating file2 from object via backendI\n";
		return {std::nullopt};
	}

	auto read_data = file2->read(file_size, 0);
	if (read_data.ptr == nullptr) {
		std::cerr << "MMIL error: reading from file2 returned nullptr\n";
		return {std::nullopt};
	}

	if (read_data.size != file_size) {
		std::cerr << "MIL error: reading from file2 size missmatch, should be " << file_size << ", is " << read_data.size << "\n";
		return {std::nullopt};
	}

	// try all loaders after another
	for (auto& il : _image_loaders) {
		auto res = il->loadFromMemoryRGBA(read_data.ptr, read_data.size);
		if (res.frames.empty() || res.height == 0 || res.width == 0) {
			continue;
		}

		TextureEntry new_entry;
		new_entry.timestamp_last_rendered = getTimeMS();
		new_entry.current_texture = 0;
		for (const auto& [ms, data] : res.frames) {
			const auto n_t = tu.upload(data.data(), res.width, res.height);
			new_entry.textures.push_back(n_t);
			new_entry.frame_duration.push_back(ms);
		}

		new_entry.width = res.width;
		new_entry.height = res.height;

		std::cout << "MIL: loaded image file o:" << /*file_path*/ entt::to_integral(o.entity()) << "\n";

		return {new_entry};
	}

	std::cerr << "MIL error: failed to load message (unhandled format)\n";
	return {std::nullopt};
}

