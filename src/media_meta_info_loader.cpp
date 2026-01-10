#include "./media_meta_info_loader.hpp"

#include "./image_loader_webp.hpp"
#include "./image_loader_sdl_bmp.hpp"
#include "./image_loader_qoi.hpp"
#include "./image_loader_sdl_image.hpp"

#include <solanaceae/message3/components.hpp>

#include "./os_comps.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <solanaceae/file/file2.hpp>

#include <limits>
#include <iostream>

void MediaMetaInfoLoader::handleMessage(const Message3Handle& m) {
	if (!static_cast<bool>(m)) {
		return;
	}

	// move to obj
	if (m.any_of<Message::Components::TagNotImage>()) {
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

	if (o.any_of<ObjComp::F::FrameDims>()) {
		return;
	}

	if (!o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		return; // we dont have all data
	}

	if (!o.all_of<ObjComp::Ephemeral::BackendFile2, ObjComp::F::SingleInfo>()) {
		std::cerr << "MMIL error: object missing file backend/file info (?)\n";
		return;
	}

	if (!o.all_of<ObjComp::F::TagLocalHaveAll>()) {
		// not ready yet
		return;
	}

	// TODO: handle collections
	const auto file_size = o.get<ObjComp::F::SingleInfo>().file_size;

	if (file_size > 50*1024*1024) {
		// not an error
		//std::cerr << "MMIL error: image file too large\n";
		// this is unlikely to change, so we tag as not image
		m.emplace<Message::Components::TagNotImage>();
		return;
	}

	if (file_size == 0) {
		std::cerr << "MMIL warning: empty file\n";
		return;
	}

	auto* file_backend = o.get<ObjComp::Ephemeral::BackendFile2>().ptr;
	if (file_backend == nullptr) {
		std::cerr << "MMIL error: object backend nullptr\n";
		return;
	}

	auto file2 = file_backend->file2(o, StorageBackendIFile2::FILE2_READ);
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

	// try all loaders after another
	for (auto& il : _image_loaders) {
		// TODO: impl callback based load
		auto res = il->loadInfoFromMemory(read_data.ptr, read_data.size);
		if (res.height == 0 || res.width == 0) {
			continue;
		}

		o.emplace<ObjComp::F::FrameDims>(
			static_cast<uint16_t>(std::min<uint32_t>(res.width, std::numeric_limits<uint16_t>::max())),
			static_cast<uint16_t>(std::min<uint32_t>(res.height, std::numeric_limits<uint16_t>::max()))
		);

		std::cout << "MMIL: loaded image file o:" << /*file_path*/ entt::to_integral(o.entity()) << "\n";

		_rmm.throwEventUpdate(m);
		return;
	}

	m.emplace<Message::Components::TagNotImage>();

	std::cout << "MMIL: loading failed image info o:" << /*file_path*/ entt::to_integral(o.entity()) << "\n";

	// TODO: update object too
	// recursion
	_rmm.throwEventUpdate(m);
}

MediaMetaInfoLoader::MediaMetaInfoLoader(RegistryMessageModelI& rmm) : _rmm(rmm), _rmm_sr(_rmm.newSubRef(this)) {
	// HACK: make them be added externally?
	_image_loaders.push_back(std::make_unique<ImageLoaderWebP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLBMP>());
	_image_loaders.push_back(std::make_unique<ImageLoaderQOI>());
	_image_loaders.push_back(std::make_unique<ImageLoaderSDLImage>());

	_rmm_sr
		.subscribe(RegistryMessageModel_Event::message_construct)
		.subscribe(RegistryMessageModel_Event::message_updated)
	;
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

