#include "./media_meta_info_loader.hpp"

#include "./image_loader_webp.hpp"
#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_qoi.hpp"
#include "./image_loader_sdl_image.hpp"

#include <solanaceae/message3/components.hpp>

#include "./os_comps.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <solanaceae/file/file2.hpp>

#include <iostream>

void MediaMetaInfoLoader::handleMessage(const Message3Handle& m) {
	if (!static_cast<bool>(m)) {
		return;
	}

	// move to obj
	if (m.any_of<Message::Components::TagNotImage, Message::Components::FrameDims>()) {
		return;
	}

	if (!m.all_of<Message::Components::MessageFileObject>()) {
		// not a file message
		return;
	}
	const auto& o = m.get<Message::Components::MessageFileObject>().o;

	if (!static_cast<bool>(o)) {
		std::cerr << "MMIL error: invalid object in file message\n";
		return;
	}

	if (!o.all_of<ObjComp::Ephemeral::Backend, ObjComp::F::SingleInfo>()) {
		std::cerr << "MMIL error: object missing backend/file info (?)\n";
		return;
	}

	// TODO: handle collections
	const auto file_size = o.get<ObjComp::F::SingleInfo>().file_size;

	if (file_size > 50*1024*1024) {
		std::cerr << "MMIL error: image file too large\n";
		return;
	}

	if (file_size == 0) {
		std::cerr << "MMIL warning: empty file\n";
		return;
	}

	if (!o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		// not ready yet
		return;
	}

	auto* file_backend = o.get<ObjComp::Ephemeral::Backend>().ptr;
	if (file_backend == nullptr) {
		std::cerr << "MMIL error: object backend nullptr\n";
		return;
	}


	auto file2 = file_backend->file2(o, StorageBackendI::FILE2_READ);
	if (!file2 || !file2->isGood() || !file2->can_read) {
		std::cerr << "MMIL error: creating file2 from object via backendI\n";
		return;
	}

	auto read_data = file2->read(file_size, 0);
	if (read_data.ptr == nullptr) {
		std::cerr << "MMIL error: reading from file2 returned nullptr\n";
		return;
	}

	if (read_data.size != file_size) {
		std::cerr << "MMIL error: reading from file2 size missmatch, should be " << file_size << ", is " << read_data.size << "\n";
		return;
	}

	//bool could_load {false};
	// try all loaders after another
	for (auto& il : _image_loaders) {
		// TODO: impl callback based load
		auto res = il->loadInfoFromMemory(read_data.ptr, read_data.size);
		if (res.height == 0 || res.width == 0) {
			continue;
		}

		m.emplace<Message::Components::FrameDims>(res.width, res.height);

		//could_load = true;

		std::cout << "MMIL: loaded image file o:" << /*file_path*/ entt::to_integral(o.entity()) << "\n";

		_rmm.throwEventUpdate(m);
		return;
	}

	//if (!could_load) {
		m.emplace<Message::Components::TagNotImage>();

		std::cout << "MMIL: loading failed image info o:" << /*file_path*/ entt::to_integral(o.entity()) << "\n";

		_rmm.throwEventUpdate(m);
	//}
}

MediaMetaInfoLoader::MediaMetaInfoLoader(RegistryMessageModel& rmm) : _rmm(rmm) {
	// HACK: make them be added externally?
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderQOI>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLImage>());

	_rmm.subscribe(this, RegistryMessageModel_Event::message_construct);
	_rmm.subscribe(this, RegistryMessageModel_Event::message_updated);
}

MediaMetaInfoLoader::~MediaMetaInfoLoader(void) {
}

bool MediaMetaInfoLoader::onEvent(const Message::Events::MessageConstruct& e) {
	handleMessage(e.e);
	return false;
}

bool MediaMetaInfoLoader::onEvent(const Message::Events::MessageUpdated& e) {
	handleMessage(e.e);
	return false;
}

