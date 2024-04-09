#include "./filesystem_storage.hpp"

#include "../meta_components.hpp"

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

namespace backend {

FilesystemStorage::FilesystemStorage(ObjectStore2& os) : StorageBackendI::StorageBackendI(os) {
}

FilesystemStorage::~FilesystemStorage(void) {
}

bool FilesystemStorage::write(Object o, std::function<write_to_storage_fetch_data_cb>& data_cb) {
	auto& reg = _os.registry();

	if (!reg.valid(o)) {
		return false;
	}

	ObjectHandle oh {reg, o};

	if (!oh.all_of<FragComp::Ephemeral::FilePath>()) {
		// not a file fragment?
		return false;
	}

	// split object storage

	MetaFileType meta_type = MetaFileType::TEXT_JSON; // TODO: better defaults
	if (reg.all_of<FragComp::Ephemeral::MetaFileType>(o)) {
		meta_type = oh.get<FragComp::Ephemeral::MetaFileType>().type;
	}

	Encryption meta_enc = Encryption::NONE; // TODO: better defaults
	Compression meta_comp = Compression::NONE; // TODO: better defaults

	if (meta_type != MetaFileType::TEXT_JSON) {
		if (oh.all_of<FragComp::Ephemeral::MetaEncryptionType>()) {
			meta_enc = oh.get<FragComp::Ephemeral::MetaEncryptionType>().enc;
		}

		if (oh.all_of<FragComp::Ephemeral::MetaCompressionType>()) {
			meta_comp = oh.get<FragComp::Ephemeral::MetaCompressionType>().comp;
		}
	} else {
		// we cant have encryption or compression
		// so we force NONE for TEXT JSON

		oh.emplace_or_replace<FragComp::Ephemeral::MetaEncryptionType>(Encryption::NONE);
		oh.emplace_or_replace<FragComp::Ephemeral::MetaCompressionType>(Compression::NONE);
	}

	std::filesystem::path meta_tmp_path = oh.get<FragComp::Ephemeral::FilePath>().path + ".meta" + metaFileTypeSuffix(meta_type) + ".tmp";
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
	if (oh.all_of<FragComp::DataEncryptionType>()) {
		data_enc = oh.get<FragComp::DataEncryptionType>().enc;
	}
	if (oh.all_of<FragComp::DataCompressionType>()) {
		data_comp = oh.get<FragComp::DataCompressionType>().comp;
	}

	std::filesystem::path data_tmp_path = oh.get<FragComp::Ephemeral::FilePath>().path + ".tmp";
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

		for (const auto& [type_id, storage] : reg.storage()) {
			if (!storage.contains(o)) {
				continue;
			}

			//std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

			// use type_id to find serializer
			auto s_cb_it = _os._sc._serl_json.find(type_id);
			if (s_cb_it == _os._sc._serl_json.end()) {
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
					std::cerr << "FS error: binary writer creation failed '" << oh.get<FragComp::Ephemeral::FilePath>().path << "'\n";
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
						std::cerr << "FS error: binary writer failed '" << oh.get<FragComp::Ephemeral::FilePath>().path << "'\n";
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

	//meta_file.flush();
	//meta_file.close();
	while (!meta_file_stack.empty()) { meta_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?
	//data_file.flush();
	//data_file.close();
	while (!data_file_stack.empty()) { data_file_stack.pop(); } // destroy stack // TODO: maybe work with scope?

	std::filesystem::rename(
		meta_tmp_path,
		oh.get<FragComp::Ephemeral::FilePath>().path + ".meta" + metaFileTypeSuffix(meta_type)
	);

	std::filesystem::rename(
		data_tmp_path,
		oh.get<FragComp::Ephemeral::FilePath>().path
	);

	// TODO: check return value of renames

	if (oh.all_of<FragComp::Ephemeral::DirtyTag>()) {
		oh.remove<FragComp::Ephemeral::DirtyTag>();
	}

	return true;
}

bool FilesystemStorage::read(Object o, std::function<read_from_storage_put_data_cb>& data_cb) {
	return false;
}

} // backend

