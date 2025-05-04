#include "./std_fs.hpp"

#include <solanaceae/object_store/meta_components.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>

#include <solanaceae/file/file2_std.hpp>

#include <iostream>

namespace Backends {

STDFS::STDFS(
	ObjectStore2& os
) : _os(os) {
}

STDFS::~STDFS(void) {
}

ObjectHandle STDFS::newObject(ByteSpan id, bool throw_construct) {
	ObjectHandle o{_os.registry(), _os.registry().create()};

	o.emplace<ObjComp::Ephemeral::BackendMeta>(this); // ?
	o.emplace<ObjComp::Ephemeral::BackendFile2>(this);
	o.emplace<ObjComp::ID>(std::vector<uint8_t>{id});
	//o.emplace<ObjComp::Ephemeral::FilePath>(object_file_path.generic_u8string());

	if (throw_construct) {
		_os.throwEventConstruct(o);
	}

	return o;
}

bool STDFS::attach(Object ov) {
	auto o = _os.objectHandle(ov);
	if (!static_cast<bool>(o)) {
		return false;
	}

	if (o.any_of<
		ObjComp::Ephemeral::BackendMeta,
		ObjComp::Ephemeral::BackendFile2
	>()) {
		return false;
	}

	o.emplace<ObjComp::Ephemeral::BackendMeta>(this); // ?
	o.emplace<ObjComp::Ephemeral::BackendFile2>(this);

	return true;
}

std::unique_ptr<File2I> STDFS::file2(Object ov, FILE2_FLAGS flags) {
	if (flags & FILE2_RAW) {
		std::cerr << "STDFS error: does not support raw modes\n";
		return nullptr;
	}

	if (flags == FILE2_NONE) {
		std::cerr << "STDFS error: no file mode set\n";
		assert(false);
		return nullptr;
	}

	ObjectHandle o{_os.registry(), ov};

	if (!static_cast<bool>(o)) {
		return nullptr;
	}

	// will this do if we go and support enc?
	// use ObjComp::Ephemeral::FilePath instead??
	if (!o.all_of<ObjComp::F::SingleInfoLocal>()) {
		std::cerr << "STDFS error: no SingleInfoLocal\n";
		return nullptr;
	}

	const auto& file_path = o.get<ObjComp::F::SingleInfoLocal>().file_path;
	if (file_path.empty()) {
		std::cerr << "STDFS error: SingleInfoLocal path empty\n";
		return nullptr;
	}

	std::unique_ptr<File2I> res;
	if ((flags & FILE2_WRITE) != 0 && (flags & FILE2_READ) != 0) {
		res = std::make_unique<File2RWFile>(file_path);
	} else if (flags & FILE2_READ) {
		res = std::make_unique<File2RFile>(file_path);
	} else if ((flags & FILE2_WRITE) && o.all_of<ObjComp::File::SingleInfo>()) {
		// HACK: use info and presize the file AND truncate
		// TODO: actually support streaming :P
		res = std::make_unique<File2RWFile>(file_path, o.get<ObjComp::File::SingleInfo>().file_size, true);
	} else if (flags & FILE2_WRITE) {
		res = std::make_unique<File2WFile>(file_path);
	}

	if (!res || !res->isGood()) {
		std::cerr << "STDFS error: failed constructing file '" << file_path << "'\n";
		return nullptr;
	}

	return res;
}

} // Backends

