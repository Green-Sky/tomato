#include "./fragment_store.hpp"

#include <solanaceae/util/utils.hpp>

#include <entt/entity/handle.hpp>
#include <entt/container/dense_set.hpp>
#include <entt/core/hashed_string.hpp>

#include <nlohmann/json.hpp>

#include <solanaceae/file/file2_std.hpp>
#include "./file2_zstd.hpp"

#include <zstd.h>

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <stack>
#include <string_view>
#include <vector>
#include <variant>

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

// TODO: these stacks are file specific
static std::stack<std::unique_ptr<File2I>> buildFileStackRead(std::string_view file_path, Encryption encryption, Compression compression) {
	std::stack<std::unique_ptr<File2I>> file_stack;
	file_stack.push(std::make_unique<File2RFile>(file_path));

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: opening file for reading '" << file_path << "'\n";
		return {};
	}

	// TODO: decrypt here
	assert(encryption == Encryption::NONE);

	// add layer based on enum
	if (compression == Compression::ZSTD) {
		file_stack.push(std::make_unique<File2ZSTDR>(*file_stack.top().get()));
	} else {
		// TODO: make error instead
		assert(compression == Compression::NONE);
	}

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: file failed to add " << (int)compression << " decompression layer '" << file_path << "'\n";
		return {};
	}

	return file_stack;
}

static std::stack<std::unique_ptr<File2I>> buildFileStackWrite(std::string_view file_path, Encryption encryption, Compression compression) {
	std::stack<std::unique_ptr<File2I>> file_stack;
	file_stack.push(std::make_unique<File2WFile>(file_path));

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: opening file for writing '" << file_path << "'\n";
		return {};
	}

	// TODO: decrypt here
	assert(encryption == Encryption::NONE);

	// add layer based on enum
	if (compression == Compression::ZSTD) {
		file_stack.push(std::make_unique<File2ZSTDW>(*file_stack.top().get()));
	} else {
		// TODO: make error instead
		assert(compression == Compression::NONE);
	}

	if (!file_stack.top()->isGood()) {
		std::cerr << "FS error: file failed to add " << (int)compression << " compression layer '" << file_path << "'\n";
		return {};
	}

	return file_stack;
}

FragmentStore::FragmentStore(void) {
	registerSerializers();
}

FragmentStore::FragmentStore(
	std::array<uint8_t, 16> session_uuid_namespace
) : _session_uuid_gen(std::move(session_uuid_namespace)) {
	registerSerializers();
}

std::vector<uint8_t> FragmentStore::generateNewUID(void) {
	return _session_uuid_gen();
}

FragmentID FragmentStore::newFragmentMemoryOwned(
	const std::vector<uint8_t>& id,
	size_t initial_size
) {
	{ // first check if id is already used
		auto exising_id = getFragmentByID(id);
		if (_reg.valid(exising_id)) {
			return entt::null;
		}
	}

	{ // next check if space in memory budget
		const auto free_memory = _memory_budget - _memory_usage;
		if (initial_size > free_memory) {
			return entt::null;
		}
	}

	// actually allocate and create
	auto new_data = std::make_unique<std::vector<uint8_t>>(initial_size);
	if (!static_cast<bool>(new_data)) {
		// allocation failure
		return entt::null;
	}
	_memory_usage += initial_size;

	const auto new_frag = _reg.create();

	_reg.emplace<FragComp::ID>(new_frag, id);
	// TODO: memory comp
	_reg.emplace<std::unique_ptr<std::vector<uint8_t>>>(new_frag) = std::move(new_data);

	throwEventConstruct(new_frag);

	return new_frag;
}

FragmentID FragmentStore::newFragmentFile(
	std::string_view store_path,
	MetaFileType mft,
	const std::vector<uint8_t>& id
) {
	{ // first check if id is already used
		const auto exising_id = getFragmentByID(id);
		if (_reg.valid(exising_id)) {
			return entt::null;
		}
	}

	if (store_path.empty()) {
		store_path = _default_store_path;
	}

	std::filesystem::create_directories(store_path);

	const auto id_hex = bin2hex(id);
	std::filesystem::path fragment_file_path;

	if (id_hex.size() < 6) {
		fragment_file_path = std::filesystem::path{store_path}/id_hex;
	} else {
		// use the first 2hex (1byte) as a subfolder
		std::filesystem::create_directories(std::string{store_path} + id_hex.substr(0, 2));
		fragment_file_path = std::filesystem::path{std::string{store_path} + id_hex.substr(0, 2)} / id_hex.substr(2);
	}

	if (std::filesystem::exists(fragment_file_path)) {
		return entt::null;
	}

	const auto new_frag = _reg.create();

	_reg.emplace<FragComp::ID>(new_frag, id);

	// file (info) comp
	_reg.emplace<FragComp::Ephemeral::FilePath>(new_frag, fragment_file_path.generic_u8string());

	_reg.emplace<FragComp::Ephemeral::MetaFileType>(new_frag, mft);

	// meta needs to be synced to file
	std::function<write_to_storage_fetch_data_cb> empty_data_cb = [](auto*, auto) -> uint64_t { return 0; };
	if (!syncToStorage(new_frag, empty_data_cb)) {
		std::cerr << "FS error: syncToStorage failed while creating new fragment file\n";
		_reg.destroy(new_frag);
		return entt::null;
	}

	// while new metadata might be created here, making sure the file could be created is more important
	throwEventConstruct(new_frag);

	return new_frag;
}
FragmentID FragmentStore::newFragmentFile(
	std::string_view store_path,
	MetaFileType mft
) {
	return newFragmentFile(store_path, mft, generateNewUID());
}

FragmentID FragmentStore::getFragmentByID(
	const std::vector<uint8_t>& id
) {
	// TODO: accelerate
	// maybe keep it sorted and binary search? hash table lookup?
	for (const auto& [frag, id_comp] : _reg.view<FragComp::ID>().each()) {
		if (id == id_comp.v) {
			return frag;
		}
	}

	return entt::null;
}

FragmentID FragmentStore::getFragmentCustomMatcher(
	std::function<bool(FragmentID)>& fn
) {
	return entt::null;
}

bool FragmentStore::syncToStorage(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb) {
	if (!_reg.valid(fid)) {
		return false;
	}

	if (!_reg.all_of<FragComp::Ephemeral::FilePath>(fid)) {
		// not a file fragment?
		return false;
	}

	// split object storage

	MetaFileType meta_type = MetaFileType::TEXT_JSON; // TODO: better defaults
	if (_reg.all_of<FragComp::Ephemeral::MetaFileType>(fid)) {
		meta_type = _reg.get<FragComp::Ephemeral::MetaFileType>(fid).type;
	}

	Encryption meta_enc = Encryption::NONE; // TODO: better defaults
	Compression meta_comp = Compression::NONE; // TODO: better defaults

	if (meta_type != MetaFileType::TEXT_JSON) {
		if (_reg.all_of<FragComp::Ephemeral::MetaEncryptionType>(fid)) {
			meta_enc = _reg.get<FragComp::Ephemeral::MetaEncryptionType>(fid).enc;
		}

		if (_reg.all_of<FragComp::Ephemeral::MetaCompressionType>(fid)) {
			meta_comp = _reg.get<FragComp::Ephemeral::MetaCompressionType>(fid).comp;
		}
	} else {
		// we cant have encryption or compression
		// so we force NONE for TEXT JSON

		_reg.emplace_or_replace<FragComp::Ephemeral::MetaEncryptionType>(fid, Encryption::NONE);
		_reg.emplace_or_replace<FragComp::Ephemeral::MetaCompressionType>(fid, Compression::NONE);
	}

	std::filesystem::path meta_tmp_path = _reg.get<FragComp::Ephemeral::FilePath>(fid).path + ".meta" + metaFileTypeSuffix(meta_type) + ".tmp";
	meta_tmp_path.replace_filename("." + meta_tmp_path.filename().generic_u8string());
	// TODO: make meta comp work with mem compressor
	//auto meta_file_stack = buildFileStackWrite(std::string_view{meta_tmp_path.generic_u8string()}, meta_enc, meta_comp);
	std::stack<std::unique_ptr<File2I>> meta_file_stack;
	meta_file_stack.push(std::make_unique<File2WFile>(std::string_view{meta_tmp_path.generic_u8string()}));

	if (!meta_file_stack.top()->isGood()) {
		std::cerr << "FS error: opening file for writing '" << meta_tmp_path << "'\n";
		return {};
	}

	if (meta_file_stack.empty()) {
		std::cerr << "FS error: failed to create temporary meta file stack\n";
		std::filesystem::remove(meta_tmp_path); // might have created an empty file
		return false;
	}

	Encryption data_enc = Encryption::NONE; // TODO: better defaults
	Compression data_comp = Compression::NONE; // TODO: better defaults
	if (_reg.all_of<FragComp::DataEncryptionType>(fid)) {
		data_enc = _reg.get<FragComp::DataEncryptionType>(fid).enc;
	}
	if (_reg.all_of<FragComp::DataCompressionType>(fid)) {
		data_comp = _reg.get<FragComp::DataCompressionType>(fid).comp;
	}

	std::filesystem::path data_tmp_path = _reg.get<FragComp::Ephemeral::FilePath>(fid).path + ".tmp";
	data_tmp_path.replace_filename("." + data_tmp_path.filename().generic_u8string());
	auto data_file_stack = buildFileStackWrite(std::string_view{data_tmp_path.generic_u8string()}, data_enc, data_comp);
	if (!data_file_stack.empty()) {
		while (!meta_file_stack.empty()) { meta_file_stack.pop(); }
		std::filesystem::remove(meta_tmp_path);
		std::cerr << "FS error: failed to create temporary data file\n";
		return false;
	}

	// sharing code between binary msgpack and text json for now
	nlohmann::json meta_data_j = nlohmann::json::object(); // metadata needs to be an object, null not allowed
	// metadata file

	for (const auto& [type_id, storage] : _reg.storage()) {
		if (!storage.contains(fid)) {
			continue;
		}

		//std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

		// use type_id to find serializer
		auto s_cb_it = _sc._serl_json.find(type_id);
		if (s_cb_it == _sc._serl_json.end()) {
			// could not find serializer, not saving
			continue;
		}

		// noooo, why cant numbers be keys
		//if (meta_type == MetaFileType::BINARY_MSGPACK) { // msgpack uses the hash id instead
			//s_cb_it->second(storage.value(fid), meta_data[storage.type().hash()]);
		//} else if (meta_type == MetaFileType::TEXT_JSON) {
		s_cb_it->second({_reg, fid}, meta_data_j[storage.type().name()]);
		//}
	}

	if (meta_type == MetaFileType::BINARY_MSGPACK) { // binary metadata file
		const std::vector<uint8_t> meta_data = nlohmann::json::to_msgpack(meta_data_j);
		std::vector<uint8_t> meta_data_compressed; // empty if none
		//std::vector<uint8_t> meta_data_encrypted; // empty if none

		if (meta_comp == Compression::ZSTD) {
			meta_data_compressed.resize(ZSTD_compressBound(meta_data.size()));

			size_t const cSize = ZSTD_compress(meta_data_compressed.data(), meta_data_compressed.size(), meta_data.data(), meta_data.size(), 0); // 0 is default is probably 3
			if (ZSTD_isError(cSize)) {
				std::cerr << "FS error: compressing meta failed\n";
				meta_data_compressed.clear();
				meta_comp = Compression::NONE;
			} else {
				meta_data_compressed.resize(cSize);
			}
		} else if (meta_comp == Compression::NONE) {
			// do nothing
		} else {
			assert(false && "implement me");
		}

		// TODO: encryption

		// the meta file is itself msgpack data
		nlohmann::json meta_header_j = nlohmann::json::array();
		meta_header_j.emplace_back() = "SOLMET";
		meta_header_j.push_back(meta_enc);
		meta_header_j.push_back(meta_comp);

		if (false) { // TODO: encryption
		} else if (!meta_data_compressed.empty()) {
			meta_header_j.push_back(nlohmann::json::binary(meta_data_compressed));
		} else {
			meta_header_j.push_back(nlohmann::json::binary(meta_data));
		}

		const auto meta_header_data = nlohmann::json::to_msgpack(meta_header_j);
		//meta_file.write(reinterpret_cast<const char*>(meta_header_data.data()), meta_header_data.size());
		meta_file_stack.top()->write({meta_header_data.data(), meta_header_data.size()});
	} else if (meta_type == MetaFileType::TEXT_JSON) {
		// cant be compressed or encrypted
		const auto meta_file_json_str = meta_data_j.dump(2, ' ', true);
		meta_file_stack.top()->write({reinterpret_cast<const uint8_t*>(meta_file_json_str.data()), meta_file_json_str.size()});
	}

	// now data
	//if (data_comp == Compression::NONE) {
		// for zstd compression, chunk size is frame size. (no cross frame referencing)
		static constexpr int64_t chunk_size{1024*1024*10};
		std::array<uint8_t, chunk_size> buffer;
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
	//} else if (data_comp == Compression::ZSTD) {
		//std::vector<uint8_t> buffer(ZSTD_CStreamInSize());
		//std::vector<uint8_t> compressed_buffer(ZSTD_CStreamOutSize());
		//uint64_t buffer_actual_size {0};

		//std::unique_ptr<ZSTD_CCtx, decltype(&ZSTD_freeCCtx)> cctx{ZSTD_createCCtx(), &ZSTD_freeCCtx};
		//ZSTD_CCtx_setParameter(cctx.get(), ZSTD_c_compressionLevel, 0); // default (3)
		//ZSTD_CCtx_setParameter(cctx.get(), ZSTD_c_checksumFlag, 1); // add extra checksums (to frames?)
		//do {
			//buffer_actual_size = data_cb(buffer.data(), buffer.size());
			////if (buffer_actual_size == 0) {
				////break;
			////}
			//if (buffer_actual_size > buffer.size()) {
				//// wtf
				//break;
			//}
			//bool const lastChunk = (buffer_actual_size < buffer.size());

			//ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;
			//ZSTD_inBuffer input = { buffer.data(), buffer_actual_size, 0 };

			//while (input.pos < input.size) {
				//ZSTD_outBuffer output = { compressed_buffer.data(), compressed_buffer.size(), 0 };

				//size_t const remaining = ZSTD_compressStream2(cctx.get(), &output , &input, mode);
				//if (ZSTD_isError(remaining)) {
					//std::cerr << "FS error: compressing data failed\n";
					//break;
				//}

				//data_file.write(reinterpret_cast<const char*>(compressed_buffer.data()), output.pos);

				//if (remaining == 0) {
					//break;
				//}
			//}
			//// same as if lastChunk break;
		//} while (buffer_actual_size == buffer.size());
	//} else {
		//assert(false && "implement me");
	//}

	//meta_file.flush();
	//meta_file.close();
	while (!meta_file_stack.empty()) { meta_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?
	//data_file.flush();
	//data_file.close();
	while (!data_file_stack.empty()) { data_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?

	std::filesystem::rename(
		meta_tmp_path,
		_reg.get<FragComp::Ephemeral::FilePath>(fid).path + ".meta" + metaFileTypeSuffix(meta_type)
	);

	std::filesystem::rename(
		data_tmp_path,
		_reg.get<FragComp::Ephemeral::FilePath>(fid).path
	);

	// TODO: check return value of renames

	if (_reg.all_of<FragComp::Ephemeral::DirtyTag>(fid)) {
		_reg.remove<FragComp::Ephemeral::DirtyTag>(fid);
	}

	return true;
}

bool FragmentStore::syncToStorage(FragmentID fid, const uint8_t* data, const uint64_t data_size) {
	std::function<FragmentStore::write_to_storage_fetch_data_cb> fn_cb = [read = 0ull, data, data_size](uint8_t* request_buffer, uint64_t buffer_size) mutable -> uint64_t {
		uint64_t i = 0;
		for (; i+read < data_size && i < buffer_size; i++) {
			request_buffer[i] = data[i+read];
		}
		read += i;

		return i;
	};
	return syncToStorage(fid, fn_cb);
}

bool FragmentStore::loadFromStorage(FragmentID fid, std::function<read_from_storage_put_data_cb>& data_cb) {
	if (!_reg.valid(fid)) {
		return false;
	}

	if (!_reg.all_of<FragComp::Ephemeral::FilePath>(fid)) {
		// not a file fragment?
		// TODO: memory fragments
		return false;
	}

	const auto& frag_path = _reg.get<FragComp::Ephemeral::FilePath>(fid).path;

	// TODO: check if metadata dirty?
	// TODO: what if file changed on disk?

	std::cout << "FS: loading fragment '" << frag_path << "'\n";

	Compression data_comp = Compression::NONE;
	if (_reg.all_of<FragComp::DataCompressionType>(fid)) {
		data_comp = _reg.get<FragComp::DataCompressionType>(fid).comp;
	}

	//std::stack<std::unique_ptr<File2I>> data_file_stack;
	//data_file_stack.push(std::make_unique<File2RFile>(std::string_view{frag_path}));

	//if (!data_file_stack.top()->isGood()) {
		//std::cerr << "FS error: fragment data file failed to open '" << frag_path << "'\n";
		//return false;
	//}

	//// TODO: decrypt here


	//// add layer based on enum
	//if (data_comp == Compression::ZSTD) {
		//data_file_stack.push(std::make_unique<File2ZSTDR>(*data_file_stack.top().get()));
	//} else {
		//// TODO: make error instead
		//assert(data_comp == Compression::NONE);
	//}

	//if (!data_file_stack.top()->isGood()) {
		//std::cerr << "FS error: fragment data file failed to add " << (int)data_comp << " decompression layer '" << frag_path << "'\n";
		//return false;
	//}
	auto data_file_stack = buildFileStackRead(std::string_view{frag_path}, Encryption::NONE, data_comp);
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

nlohmann::json FragmentStore::loadFromStorageNJ(FragmentID fid) {
	std::vector<uint8_t> tmp_buffer;
	std::function<read_from_storage_put_data_cb> cb = [&tmp_buffer](const ByteSpan buffer) {
		tmp_buffer.insert(tmp_buffer.end(), buffer.cbegin(), buffer.cend());
	};

	if (!loadFromStorage(fid, cb)) {
		return nullptr;
	}

	return nlohmann::json::parse(tmp_buffer);
}

size_t FragmentStore::scanStoragePath(std::string_view path) {
	if (path.empty()) {
		path = _default_store_path;
	}
	// TODO: extract so async can work (or/and make iteratable generator)

	if (!std::filesystem::is_directory(path)) {
		std::cerr << "FS error: scan path not a directory '" << path << "'\n";
		return 0;
	}

	// step 1: make snapshot of files, validate metafiles and save id/path+meta.ext
	// can be extra thread (if non vfs)
	struct FragFileEntry {
		std::string id_str;
		std::filesystem::path frag_path;
		std::string meta_ext;

		bool operator==(const FragFileEntry& other) const {
			// only compare by id
			return id_str == other.id_str;
		}
	};
	struct FragFileEntryHash {
		size_t operator()(const FragFileEntry& it) const {
			return entt::hashed_string(it.id_str.data(), it.id_str.size());
		}
	};
	entt::dense_set<FragFileEntry, FragFileEntryHash> file_frag_list;

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
			std::cerr << "FS error: non hex fragment uid detected: '" << id_str << "'\n";
		}

		if (file_frag_list.contains(FragFileEntry{id_str, {}, ""})) {
			std::cerr << "FS error: fragment duplicate detected: '" << id_str << "'\n";
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
				std::cerr << "FS error: fragment with multiple meta files detected: " << id_str << "\n";
				return; // skip
			}

			if (meta_sum == 0) {
				std::cerr << "FS error: fragment missing meta file detected: " << id_str << "\n";
				return; // skip
			}

			if (has_meta_json) {
				meta_ext = ".meta.json";
			}
		}

		file_frag_list.emplace(FragFileEntry{
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
	for (const auto& it : file_frag_list) {
		std::cout << "  " << it.id_str << "\n";
	}

	// step 2: check if files preexist in reg
	// main thread
	// (merge into step 3 and do more error checking?)
	for (auto it = file_frag_list.begin(); it != file_frag_list.end();) {
		auto id = hex2bin(it->id_str);
		auto fid = getFragmentByID(id);
		if (_reg.valid(fid)) {
			// pre exising (handle differently??)
			// check if store differs?
			it = file_frag_list.erase(it);
		} else {
			it++;
		}
	}

	std::vector<FragmentID> scanned_frags;
	// step 3: parse meta and insert into reg of non preexising
	// main thread
	// TODO: check timestamps of preexisting and reload? mark external/remote dirty?
	for (const auto& it : file_frag_list) {
		nlohmann::json j;
		if (it.meta_ext == ".meta.msgpack") {
			std::ifstream file(it.frag_path.generic_u8string() + it.meta_ext, std::ios::in | std::ios::binary);
			if (!file.is_open()) {
				std::cout << "FS error: failed opening meta " << it.frag_path << "\n";
				continue;
			}

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

			const auto meta_header_j = nlohmann::json::from_msgpack(full_meta_data);

			if (!meta_header_j.is_array() || meta_header_j.size() < 4) {
				std::cerr << "FS error: broken binary meta " << it.frag_path << "\n";
				continue;
			}

			if (meta_header_j.at(0) != "SOLMET") {
				std::cerr << "FS error: wrong magic '" << meta_header_j.at(0) << "' in meta " << it.frag_path << "\n";
				continue;
			}

			Encryption meta_enc = meta_header_j.at(1);
			if (meta_enc != Encryption::NONE) {
				std::cerr << "FS error: unknown encryption " << it.frag_path << "\n";
				continue;
			}

			Compression meta_comp = meta_header_j.at(2);
			if (meta_comp != Compression::NONE && meta_comp != Compression::ZSTD) {
				std::cerr << "FS error: unknown compression " << it.frag_path << "\n";
				continue;
			}

			//const auto& meta_data_ref = meta_header_j.at(3).is_binary()?meta_header_j.at(3):meta_header_j.at(3).at("data");
			if (!meta_header_j.at(3).is_binary()) {
				std::cerr << "FS error: meta data not binary " << it.frag_path << "\n";
				continue;
			}
			const nlohmann::json::binary_t& meta_data_ref = meta_header_j.at(3);

			std::vector<uint8_t> meta_data_decomp;
			if (meta_comp == Compression::NONE) {
				// do nothing
			} else if (meta_comp == Compression::ZSTD) {
				meta_data_decomp.resize(ZSTD_DStreamOutSize());
				std::unique_ptr<ZSTD_DCtx, decltype(&ZSTD_freeDCtx)> dctx{ZSTD_createDCtx(), &ZSTD_freeDCtx};

				ZSTD_inBuffer input {meta_data_ref.data(), meta_data_ref.size(), 0};
				ZSTD_outBuffer output = {meta_data_decomp.data(), meta_data_decomp.size(), 0};
				do {
					size_t const ret = ZSTD_decompressStream(dctx.get(), &output , &input);
					if (ZSTD_isError(ret)) {
						// error <.<
						std::cerr << "FS error: decompression error\n";
						//meta_data_decomp.clear();
						output.pos = 0; // resize after loop
						break;
					}
				} while (input.pos < input.size);
				meta_data_decomp.resize(output.pos);
			} else {
				assert(false && "implement me");
			}

			// TODO: enc

			if (!meta_data_decomp.empty()) {
				j = nlohmann::json::from_msgpack(meta_data_decomp);
			} else {
				j = nlohmann::json::from_msgpack(meta_data_ref);
			}
		} else if (it.meta_ext == ".meta.json") {
			std::ifstream file(it.frag_path.generic_u8string() + it.meta_ext, std::ios::in | std::ios::binary);
			if (!file.is_open()) {
				std::cerr << "FS error: failed opening meta " << it.frag_path << "\n";
				continue;
			}

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
		FragmentHandle fh{_reg, _reg.create()};
		fh.emplace<FragComp::ID>(hex2bin(it.id_str));

		fh.emplace<FragComp::Ephemeral::FilePath>(it.frag_path.generic_u8string());

		for (const auto& [k, v] : j.items()) {
			// type id from string hash
			const auto type_id = entt::hashed_string(k.data(), k.size());
			const auto deserl_fn_it = _sc._deserl_json.find(type_id);
			if (deserl_fn_it != _sc._deserl_json.cend()) {
				// TODO: check return value
				deserl_fn_it->second(fh, v);
			} else {
				std::cerr << "FS warning: missing deserializer for meta key '" << k << "'\n";
			}
		}
		scanned_frags.push_back(fh);
	}

	// TODO: mutex and move code to async and return this list ?

	// throw new frag event here, after loading them all
	for (const FragmentID fid : scanned_frags) {
		throwEventConstruct(fid);
	}

	return scanned_frags.size();
}

void FragmentStore::scanStoragePathAsync(std::string path) {
	// add path to queue
	// HACK: if path is known/any fragment is in the path, this operation blocks (non async)
	scanStoragePath(path); // TODO: make async and post result
}

static bool serl_json_data_enc_type(const FragmentHandle fh, nlohmann::json& out) {
	out = static_cast<std::underlying_type_t<Encryption>>(
		fh.get<FragComp::DataEncryptionType>().enc
	);
	return true;
}

static bool deserl_json_data_enc_type(FragmentHandle fh, const nlohmann::json& in) {
	fh.emplace_or_replace<FragComp::DataEncryptionType>(
		static_cast<Encryption>(
			static_cast<std::underlying_type_t<Encryption>>(in)
		)
	);
	return true;
}

static bool serl_json_data_comp_type(const FragmentHandle fh, nlohmann::json& out) {
	out = static_cast<std::underlying_type_t<Compression>>(
		fh.get<FragComp::DataCompressionType>().comp
	);
	return true;
}

static bool deserl_json_data_comp_type(FragmentHandle fh, const nlohmann::json& in) {
	fh.emplace_or_replace<FragComp::DataCompressionType>(
		static_cast<Compression>(
			static_cast<std::underlying_type_t<Compression>>(in)
		)
	);
	return true;
}

void FragmentStore::registerSerializers(void) {
	_sc.registerSerializerJson<FragComp::DataEncryptionType>(serl_json_data_enc_type);
	_sc.registerDeSerializerJson<FragComp::DataEncryptionType>(deserl_json_data_enc_type);
	_sc.registerSerializerJson<FragComp::DataCompressionType>(serl_json_data_comp_type);
	_sc.registerDeSerializerJson<FragComp::DataCompressionType>(deserl_json_data_comp_type);
}

