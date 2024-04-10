#include "./filesystem_storage.hpp"

#include "../meta_components.hpp"
#include "../serializer_json.hpp"

#include <solanaceae/util/utils.hpp>

#include <entt/container/dense_set.hpp>
#include <nlohmann/json.hpp>

#include <solanaceae/file/file2_std.hpp>
#include <solanaceae/file/file2_mem.hpp>

#include "../file2_stack.hpp"

#include <filesystem>

#include <iostream>

static const char* metaFileTypeSuffix(MetaFileType mft) {
	switch (mft) {
		case MetaFileType::TEXT_JSON: return ".json";
		//case MetaFileType::BINARY_ARB: return ".bin";
		case MetaFileType::BINARY_MSGPACK: return ".msgpack";
	}
	return ""; // .unk?
}

// TODO: move to ... somewhere. (span? file2i?)
static ByteSpan spanFromRead(const std::variant<ByteSpan, std::vector<uint8_t>>& data_var) {
	if (std::holds_alternative<std::vector<uint8_t>>(data_var)) {
		auto& vec = std::get<std::vector<uint8_t>>(data_var);
		return {vec.data(), vec.size()};
	} else if (std::holds_alternative<ByteSpan>(data_var)) {
		return std::get<ByteSpan>(data_var);
	} else {
		assert(false);
		return {};
	}
}


namespace backend {

FilesystemStorage::FilesystemStorage(ObjectStore2& os, std::string_view storage_path) : StorageBackendI::StorageBackendI(os), _storage_path(storage_path) {
}

FilesystemStorage::~FilesystemStorage(void) {
}

ObjectHandle FilesystemStorage::newObject(MetaFileType mft, ByteSpan id) {
	{ // first check if id is already used (TODO: solve the multi obj/backend problem)
		auto exising_oh = _os.getOneObjectByID(id);
		if (static_cast<bool>(exising_oh)) {
			return {};
		}
	}

	std::filesystem::create_directories(_storage_path);

	if (!std::filesystem::is_directory(_storage_path)) {
		std::cerr << "FS error: failed to create storage path dir '" << _storage_path << "'\n";
		return {};
	}

	const auto id_hex = bin2hex(id);
	std::filesystem::path object_file_path;

	// TODO: refactor this magic somehow
	if (id_hex.size() < 6) {
		object_file_path = std::filesystem::path{_storage_path}/id_hex;
	} else {
		// use the first 2hex (1byte) as a subfolder
		std::filesystem::create_directories(std::string{_storage_path} + id_hex.substr(0, 2));
		object_file_path = std::filesystem::path{std::string{_storage_path} + id_hex.substr(0, 2)} / id_hex.substr(2);
	}

	if (std::filesystem::exists(object_file_path)) {
		std::cerr << "FS error: object already exists in path '" << id_hex << "'\n";
		return {};
	}

	ObjectHandle oh{_os.registry(), _os.registry().create()};

	oh.emplace<ObjComp::Ephemeral::Backend>(this);
	oh.emplace<ObjComp::ID>(std::vector<uint8_t>{id});
	oh.emplace<ObjComp::Ephemeral::FilePath>(object_file_path.generic_u8string());
	oh.emplace<ObjComp::Ephemeral::MetaFileType>(mft);

	// meta needs to be synced to file
	std::function<write_to_storage_fetch_data_cb> empty_data_cb = [](auto*, auto) -> uint64_t { return 0; };
	if (!write(oh, empty_data_cb)) {
		std::cerr << "FS error: write failed while creating new object file\n";
		oh.destroy();
		return {};
	}

	// while new metadata might be created here, making sure the file could be created is more important
	_os.throwEventConstruct(oh);

	return oh;
}

bool FilesystemStorage::write(Object o, std::function<write_to_storage_fetch_data_cb>& data_cb) {
	auto& reg = _os.registry();

	if (!reg.valid(o)) {
		return false;
	}

	ObjectHandle oh {reg, o};

	if (!oh.all_of<ObjComp::Ephemeral::FilePath>()) {
		// not a file fragment?
		return false;
	}

	// split object storage (meta and data are 2 files)

	MetaFileType meta_type = MetaFileType::TEXT_JSON; // TODO: better defaults?
	if (reg.all_of<ObjComp::Ephemeral::MetaFileType>(o)) {
		meta_type = oh.get<ObjComp::Ephemeral::MetaFileType>().type;
	}

	Encryption meta_enc = Encryption::NONE; // TODO: better defaults?
	Compression meta_comp = Compression::NONE; // TODO: better defaults?

	if (meta_type != MetaFileType::TEXT_JSON) {
		if (oh.all_of<ObjComp::Ephemeral::MetaEncryptionType>()) {
			meta_enc = oh.get<ObjComp::Ephemeral::MetaEncryptionType>().enc;
		}

		if (oh.all_of<ObjComp::Ephemeral::MetaCompressionType>()) {
			meta_comp = oh.get<ObjComp::Ephemeral::MetaCompressionType>().comp;
		}
	} else {
		// we cant have encryption or compression
		// so we force NONE for TEXT JSON

		oh.emplace_or_replace<ObjComp::Ephemeral::MetaEncryptionType>(Encryption::NONE);
		oh.emplace_or_replace<ObjComp::Ephemeral::MetaCompressionType>(Compression::NONE);
	}

	std::filesystem::path meta_tmp_path = oh.get<ObjComp::Ephemeral::FilePath>().path + ".meta" + metaFileTypeSuffix(meta_type) + ".tmp";
	meta_tmp_path.replace_filename("." + meta_tmp_path.filename().generic_u8string());
	// TODO: make meta comp work with mem compressor
	//auto meta_file_stack = buildFileStackWrite(std::string_view{meta_tmp_path.generic_u8string()}, meta_enc, meta_comp);
	std::stack<std::unique_ptr<File2I>> meta_file_stack;
	meta_file_stack.push(std::make_unique<File2WFile>(std::string_view{meta_tmp_path.generic_u8string()}));

	if (meta_file_stack.empty()) {
		std::cerr << "FS error: failed to create temporary meta file stack\n";
		std::filesystem::remove(meta_tmp_path); // might have created an empty file
		return false;
	}

	Encryption data_enc = Encryption::NONE; // TODO: better defaults
	Compression data_comp = Compression::NONE; // TODO: better defaults
	if (oh.all_of<ObjComp::DataEncryptionType>()) {
		data_enc = oh.get<ObjComp::DataEncryptionType>().enc;
	}
	if (oh.all_of<ObjComp::DataCompressionType>()) {
		data_comp = oh.get<ObjComp::DataCompressionType>().comp;
	}

	std::filesystem::path data_tmp_path = oh.get<ObjComp::Ephemeral::FilePath>().path + ".tmp";
	data_tmp_path.replace_filename("." + data_tmp_path.filename().generic_u8string());
	auto data_file_stack = buildFileStackWrite(std::string_view{data_tmp_path.generic_u8string()}, data_enc, data_comp);
	if (data_file_stack.empty()) {
		while (!meta_file_stack.empty()) { meta_file_stack.pop(); }
		std::filesystem::remove(meta_tmp_path);
		std::cerr << "FS error: failed to create temporary data file stack\n";
		return false;
	}

	try { // TODO: properly sanitize strings, so this cant throw
		// sharing code between binary msgpack and text json for now
		nlohmann::json meta_data_j = nlohmann::json::object(); // metadata needs to be an object, null not allowed
		// metadata file

		auto& sjc = _os.registry().ctx().get<SerializerJsonCallbacks<Object>>();

		// TODO: refactor extract to OS
		for (const auto& [type_id, storage] : reg.storage()) {
			if (!storage.contains(o)) {
				continue;
			}

			//std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

			// use type_id to find serializer
			auto s_cb_it = sjc._serl.find(type_id);
			if (s_cb_it == sjc._serl.end()) {
				// could not find serializer, not saving
				continue;
			}

			// noooo, why cant numbers be keys
			//if (meta_type == MetaFileType::BINARY_MSGPACK) { // msgpack uses the hash id instead
				//s_cb_it->second(storage.value(fid), meta_data[storage.type().hash()]);
			//} else if (meta_type == MetaFileType::TEXT_JSON) {
			s_cb_it->second(oh, meta_data_j[storage.type().name()]);
			//}
		}

		if (meta_type == MetaFileType::BINARY_MSGPACK) { // binary metadata file
			std::vector<uint8_t> binary_meta_data;
			{
				std::stack<std::unique_ptr<File2I>> binary_writer_stack;
				binary_writer_stack.push(std::make_unique<File2MemW>(binary_meta_data));

				if (!buildStackWrite(binary_writer_stack, meta_enc, meta_comp)) {
					while (!meta_file_stack.empty()) { meta_file_stack.pop(); }
					std::filesystem::remove(meta_tmp_path);
					while (!data_file_stack.empty()) { data_file_stack.pop(); }
					std::filesystem::remove(data_tmp_path);
					std::cerr << "FS error: binary writer creation failed '" << oh.get<ObjComp::Ephemeral::FilePath>().path << "'\n";
					return false;
				}

				{
					const std::vector<uint8_t> meta_data = nlohmann::json::to_msgpack(meta_data_j);
					if (!binary_writer_stack.top()->write(ByteSpan{meta_data})) {
						// i feel like exceptions or refactoring would be nice here
						while (!meta_file_stack.empty()) { meta_file_stack.pop(); }
						std::filesystem::remove(meta_tmp_path);
						while (!data_file_stack.empty()) { data_file_stack.pop(); }
						std::filesystem::remove(data_tmp_path);
						std::cerr << "FS error: binary writer failed '" << oh.get<ObjComp::Ephemeral::FilePath>().path << "'\n";
						return false;
					}
				}
			}

			//// the meta file is itself msgpack data
			nlohmann::json meta_header_j = nlohmann::json::array();
			meta_header_j.emplace_back() = "SOLMET";
			meta_header_j.push_back(meta_enc);
			meta_header_j.push_back(meta_comp);

			// with a custom msgpack impl like cmp, we can be smarter here and dont need an extra buffer
			meta_header_j.push_back(nlohmann::json::binary(binary_meta_data));

			const auto meta_header_data = nlohmann::json::to_msgpack(meta_header_j);
			meta_file_stack.top()->write({meta_header_data.data(), meta_header_data.size()});
		} else if (meta_type == MetaFileType::TEXT_JSON) {
			// cant be compressed or encrypted
			const auto meta_file_json_str = meta_data_j.dump(2, ' ', true);
			meta_file_stack.top()->write({reinterpret_cast<const uint8_t*>(meta_file_json_str.data()), meta_file_json_str.size()});
		}

	} catch (...) {
		while (!meta_file_stack.empty()) { meta_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?
		std::filesystem::remove(meta_tmp_path);
		while (!data_file_stack.empty()) { data_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?
		std::filesystem::remove(data_tmp_path);
		std::cerr << "FS error: failed to serialize json data\n";
		return false;
	}

	// now data
	// for zstd compression, chunk size is frame size. (no cross frame referencing)
	// TODO: add buffering steam layer
	static constexpr int64_t chunk_size{1024*1024}; // 1MiB should be enough
	std::vector<uint8_t> buffer(chunk_size);
	uint64_t buffer_actual_size {0};
	do {
		buffer_actual_size = data_cb(buffer.data(), buffer.size());
		if (buffer_actual_size == 0) {
			break;
		}
		if (buffer_actual_size > buffer.size()) {
			// wtf
			break;
		}

		data_file_stack.top()->write({buffer.data(), buffer_actual_size});
	} while (buffer_actual_size == buffer.size());

	// flush // TODO: use scope
	while (!meta_file_stack.empty()) { meta_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?
	while (!data_file_stack.empty()) { data_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?

	std::filesystem::rename(
		meta_tmp_path,
		oh.get<ObjComp::Ephemeral::FilePath>().path + ".meta" + metaFileTypeSuffix(meta_type)
	);

	std::filesystem::rename(
		data_tmp_path,
		oh.get<ObjComp::Ephemeral::FilePath>().path
	);

	// TODO: check return value of renames

	if (oh.all_of<ObjComp::Ephemeral::DirtyTag>()) {
		oh.remove<ObjComp::Ephemeral::DirtyTag>();
	}

	return true;
}

bool FilesystemStorage::read(Object o, std::function<read_from_storage_put_data_cb>& data_cb) {
	auto& reg = _os.registry();

	if (!reg.valid(o)) {
		return false;
	}

	ObjectHandle oh {reg, o};

	if (!oh.all_of<ObjComp::Ephemeral::FilePath>()) {
		// not a file
		return false;
	}

	const auto& obj_path = oh.get<ObjComp::Ephemeral::FilePath>().path;

	// TODO: check if metadata dirty?
	// TODO: what if file changed on disk?

	std::cout << "FS: loading fragment '" << obj_path << "'\n";

	Compression data_comp = Compression::NONE;
	if (oh.all_of<ObjComp::DataCompressionType>()) {
		data_comp = oh.get<ObjComp::DataCompressionType>().comp;
	}

	auto data_file_stack = buildFileStackRead(std::string_view{obj_path}, Encryption::NONE, data_comp);
	if (data_file_stack.empty()) {
		return false;
	}

	// TODO: make it read in a single chunk instead?
	static constexpr int64_t chunk_size {1024 * 1024}; // 1MiB should be good for read
	do {
		auto data_var = data_file_stack.top()->read(chunk_size);
		ByteSpan data = spanFromRead(data_var);

		if (data.empty()) {
			// error or probably eof
			break;
		}

		data_cb(data);

		if (data.size < chunk_size) {
			// eof
			break;
		}
	} while (data_file_stack.top()->isGood());

	return true;
}

void FilesystemStorage::scanAsync(void) {
	scanPathAsync(_storage_path);
}

size_t FilesystemStorage::scanPath(std::string_view path) {
	// TODO: extract so async can work (or/and make iteratable generator)

	if (path.empty() || !std::filesystem::is_directory(path)) {
		std::cerr << "FS error: scan path not a directory '" << path << "'\n";
		return 0;
	}

	// step 1: make snapshot of files, validate metafiles and save id/path+meta.ext
	// can be extra thread (if non vfs)
	struct ObjFileEntry {
		std::string id_str;
		std::filesystem::path obj_path;
		std::string meta_ext;

		bool operator==(const ObjFileEntry& other) const {
			// only compare by id
			return id_str == other.id_str;
		}
	};
	struct ObjFileEntryHash {
		size_t operator()(const ObjFileEntry& it) const {
			return entt::hashed_string(it.id_str.data(), it.id_str.size());
		}
	};
	entt::dense_set<ObjFileEntry, ObjFileEntryHash> file_obj_list;

	std::filesystem::path storage_path{path};

	auto handle_file = [&](const std::filesystem::path& file_path) {
		if (!std::filesystem::is_regular_file(file_path)) {
			return;
		}
		// handle file

		if (file_path.has_extension()) {
			// skip over metadata, assuming only metafiles have extentions (might be wrong?)
			// also skips temps
			return;
		}

		auto relative_path = std::filesystem::proximate(file_path, storage_path);
		std::string id_str = relative_path.generic_u8string();
		// delete all '/'
		id_str.erase(std::remove(id_str.begin(), id_str.end(), '/'), id_str.end());
		if (id_str.size() % 2 != 0) {
			std::cerr << "FS error: non hex object uid detected: '" << id_str << "'\n";
		}

		if (file_obj_list.contains(ObjFileEntry{id_str, {}, ""})) {
			std::cerr << "FS error: object duplicate detected: '" << id_str << "'\n";
			return; // skip
		}

		const char* meta_ext = ".meta.msgpack";
		{ // find meta
			// TODO: this as to know all possible extentions
			bool has_meta_msgpack = std::filesystem::is_regular_file(file_path.generic_u8string() + ".meta.msgpack");
			bool has_meta_json = std::filesystem::is_regular_file(file_path.generic_u8string() + ".meta.json");
			const size_t meta_sum =
				(has_meta_msgpack?1:0) +
				(has_meta_json?1:0)
			;

			if (meta_sum > 1) { // has multiple
				std::cerr << "FS error: object with multiple meta files detected: " << id_str << "\n";
				return; // skip
			}

			if (meta_sum == 0) {
				std::cerr << "FS error: object missing meta file detected: " << id_str << "\n";
				return; // skip
			}

			if (has_meta_json) {
				meta_ext = ".meta.json";
			}
		}

		file_obj_list.emplace(ObjFileEntry{
			std::move(id_str),
			file_path,
			meta_ext
		});
	};

	for (const auto& outer_path : std::filesystem::directory_iterator(storage_path)) {
		if (std::filesystem::is_regular_file(outer_path)) {
			handle_file(outer_path);
		} else if (std::filesystem::is_directory(outer_path)) {
			// subdir, part of id
			for (const auto& inner_path : std::filesystem::directory_iterator(outer_path)) {
				//if (std::filesystem::is_regular_file(inner_path)) {

					//// handle file
				//} // TODO: support deeper recursion?
				handle_file(inner_path);
			}
		}
	}

	std::cout << "FS: scan found:\n";
	for (const auto& it : file_obj_list) {
		std::cout << "  " << it.id_str << "\n";
	}

	// step 2: check if files preexist in reg
	// main thread
	// (merge into step 3 and do more error checking?)
	// TODO: properly handle duplicates, dups form different backends might be legit
	// important
	for (auto it = file_obj_list.begin(); it != file_obj_list.end();) {
		auto id = hex2bin(it->id_str);
		auto oh = _os.getOneObjectByID(ByteSpan{id});
		if (static_cast<bool>(oh)) {
			// pre exising (handle differently??)
			// check if store differs?
			it = file_obj_list.erase(it);
		} else {
			it++;
		}
	}

	auto& sjc = _os.registry().ctx().get<SerializerJsonCallbacks<Object>>();

	std::vector<Object> scanned_objs;
	// step 3: parse meta and insert into reg of non preexising
	// main thread
	// TODO: check timestamps of preexisting and reload? mark external/remote dirty?
	for (const auto& it : file_obj_list) {
		MetaFileType mft {MetaFileType::TEXT_JSON};
		Encryption meta_enc {Encryption::NONE};
		Compression meta_comp {Compression::NONE};
		nlohmann::json j;
		if (it.meta_ext == ".meta.msgpack") {
			std::ifstream file(it.obj_path.generic_u8string() + it.meta_ext, std::ios::in | std::ios::binary);
			if (!file.is_open()) {
				std::cout << "FS error: failed opening meta " << it.obj_path << "\n";
				continue;
			}

			mft = MetaFileType::BINARY_MSGPACK;

			// file is a msgpack within a msgpack

			std::vector<uint8_t> full_meta_data;
			{ // read meta file
				// figure out size
				file.seekg(0, file.end);
				uint64_t file_size = file.tellg();
				file.seekg(0, file.beg);

				full_meta_data.resize(file_size);

				file.read(reinterpret_cast<char*>(full_meta_data.data()), full_meta_data.size());
			}

			const auto meta_header_j = nlohmann::json::from_msgpack(full_meta_data, true, false);

			if (!meta_header_j.is_array() || meta_header_j.size() < 4) {
				std::cerr << "FS error: broken binary meta " << it.obj_path << "\n";
				continue;
			}

			if (meta_header_j.at(0) != "SOLMET") {
				std::cerr << "FS error: wrong magic '" << meta_header_j.at(0) << "' in meta " << it.obj_path << "\n";
				continue;
			}

			meta_enc = meta_header_j.at(1);
			if (meta_enc != Encryption::NONE) {
				std::cerr << "FS error: unknown encryption " << it.obj_path << "\n";
				continue;
			}

			meta_comp = meta_header_j.at(2);
			if (meta_comp != Compression::NONE && meta_comp != Compression::ZSTD) {
				std::cerr << "FS error: unknown compression " << it.obj_path << "\n";
				continue;
			}

			//const auto& meta_data_ref = meta_header_j.at(3).is_binary()?meta_header_j.at(3):meta_header_j.at(3).at("data");
			if (!meta_header_j.at(3).is_binary()) {
				std::cerr << "FS error: meta data not binary " << it.obj_path << "\n";
				continue;
			}
			const nlohmann::json::binary_t& meta_data_ref = meta_header_j.at(3);

			std::stack<std::unique_ptr<File2I>> binary_reader_stack;
			binary_reader_stack.push(std::make_unique<File2MemR>(ByteSpan{meta_data_ref.data(), meta_data_ref.size()}));

			if (!buildStackRead(binary_reader_stack, meta_enc, meta_comp)) {
				std::cerr << "FS error: binary reader creation failed " << it.obj_path << "\n";
				continue;
			}

			// HACK: read fixed amout of data, but this way if we have neither enc nor comp we pass the span through
			auto binary_read_value = binary_reader_stack.top()->read(10*1024*1024); // is 10MiB large enough for meta?
			const auto binary_read_span = spanFromRead(binary_read_value);
			assert(binary_read_span.size < 10*1024*1024);

			j = nlohmann::json::from_msgpack(binary_read_span, true, false);
		} else if (it.meta_ext == ".meta.json") {
			std::ifstream file(it.obj_path.generic_u8string() + it.meta_ext, std::ios::in | std::ios::binary);
			if (!file.is_open()) {
				std::cerr << "FS error: failed opening meta " << it.obj_path << "\n";
				continue;
			}

			mft = MetaFileType::TEXT_JSON;

			file >> j;
		} else {
			assert(false);
		}

		if (!j.is_object()) {
			std::cerr << "FS error: json in meta is broken " << it.id_str << "\n";
			continue;
		}

		// TODO: existing fragment file
		//newFragmentFile();
		ObjectHandle oh{_os.registry(), _os.registry().create()};
		oh.emplace<ObjComp::Ephemeral::Backend>(this);
		oh.emplace<ObjComp::ID>(hex2bin(it.id_str));
		oh.emplace<ObjComp::Ephemeral::MetaFileType>(mft);
		oh.emplace<ObjComp::Ephemeral::MetaEncryptionType>(meta_enc);
		oh.emplace<ObjComp::Ephemeral::MetaCompressionType>(meta_comp);

		oh.emplace<ObjComp::Ephemeral::FilePath>(it.obj_path.generic_u8string());

		for (const auto& [k, v] : j.items()) {
			// type id from string hash
			const auto type_id = entt::hashed_string(k.data(), k.size());
			const auto deserl_fn_it = sjc._deserl.find(type_id);
			if (deserl_fn_it != sjc._deserl.cend()) {
				// TODO: check return value
				deserl_fn_it->second(oh, v);
			} else {
				std::cerr << "FS warning: missing deserializer for meta key '" << k << "'\n";
			}
		}
		scanned_objs.push_back(oh);
	}

	// TODO: mutex and move code to async and return this list ?

	// throw new frag event here, after loading them all
	for (const Object o : scanned_objs) {
		_os.throwEventConstruct(o);
	}

	return scanned_objs.size();
}

void FilesystemStorage::scanPathAsync(std::string path) {
	// add path to queue
	// HACK: if path is known/any fragment is in the path, this operation blocks (non async)
	scanPath(path); // TODO: make async and post result
}

} // backend

