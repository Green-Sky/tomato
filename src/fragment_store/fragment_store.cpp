#include "./fragment_store.hpp"

#include <solanaceae/util/utils.hpp>

#include <entt/entity/handle.hpp>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#include <iostream>
#include <vector>

static const char* metaFileTypeSuffix(MetaFileType mft) {
	switch (mft) {
		case MetaFileType::TEXT_JSON: return ".json";
		//case MetaFileType::BINARY_ARB: return ".bin";
		case MetaFileType::BINARY_MSGPACK: return ".msgpack";
	}
	return ""; // .unk?
}

FragmentStore::FragmentStore(void) {
	{ // random namespace
		const auto num0 = _rng();
		const auto num1 = _rng();

		_session_uuid_namespace[0] = (num0 >> 0) & 0xff;
		_session_uuid_namespace[1] = (num0 >> 8) & 0xff;
		_session_uuid_namespace[2] = (num0 >> 16) & 0xff;
		_session_uuid_namespace[3] = (num0 >> 24) & 0xff;

		_session_uuid_namespace[4] = (num1 >> 0) & 0xff;
		_session_uuid_namespace[5] = (num1 >> 8) & 0xff;
		_session_uuid_namespace[6] = (num1 >> 16) & 0xff;
		_session_uuid_namespace[7] = (num1 >> 24) & 0xff;

	}
	registerSerializers();
}

FragmentStore::FragmentStore(
	std::array<uint8_t, 8> session_uuid_namespace
) : _session_uuid_namespace(std::move(session_uuid_namespace)) {
	registerSerializers();
}

entt::basic_handle<entt::basic_registry<FragmentID>> FragmentStore::fragmentHandle(FragmentID fid) {
	return {_reg, fid};
}

std::vector<uint8_t> FragmentStore::generateNewUID(std::array<uint8_t, 8>& uuid_namespace) {
	std::vector<uint8_t> new_uid(uuid_namespace.cbegin(), uuid_namespace.cend());
	new_uid.resize(new_uid.size() + 8);

	const auto num0 = _rng();
	const auto num1 = _rng();

	new_uid[uuid_namespace.size()+0] = (num0 >> 0) & 0xff;
	new_uid[uuid_namespace.size()+1] = (num0 >> 8) & 0xff;
	new_uid[uuid_namespace.size()+2] = (num0 >> 16) & 0xff;
	new_uid[uuid_namespace.size()+3] = (num0 >> 24) & 0xff;

	new_uid[uuid_namespace.size()+4] = (num1 >> 0) & 0xff;
	new_uid[uuid_namespace.size()+5] = (num1 >> 8) & 0xff;
	new_uid[uuid_namespace.size()+6] = (num1 >> 16) & 0xff;
	new_uid[uuid_namespace.size()+7] = (num1 >> 24) & 0xff;

	return new_uid;
}

std::vector<uint8_t> FragmentStore::generateNewUID(void) {
	return generateNewUID(_session_uuid_namespace);
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

	_reg.emplace<Components::ID>(new_frag, id);
	// TODO: memory comp
	_reg.emplace<std::unique_ptr<std::vector<uint8_t>>>(new_frag) = std::move(new_data);

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

	_reg.emplace<Components::ID>(new_frag, id);

	// file (info) comp
	_reg.emplace<Components::Ephemeral::FilePath>(new_frag, fragment_file_path.generic_u8string());

	_reg.emplace<Components::Ephemeral::MetaFileType>(new_frag, mft);

	// meta needs to be synced to file
	std::function<write_to_storage_fetch_data_cb> empty_data_cb = [](const uint8_t*, uint64_t) -> uint64_t { return 0; };
	if (!syncToStorage(new_frag, empty_data_cb)) {
		_reg.destroy(new_frag);
		return entt::null;
	}

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
	for (const auto& [frag, id_comp] : _reg.view<Components::ID>().each()) {
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

template<typename F>
static void writeBinaryMetafileHeader(F& file, const Encryption enc, const Compression comp) {
	file.write("SOLMET", 6);
	file.put(static_cast<std::underlying_type_t<Encryption>>(enc));

	// TODO: is compressiontype encrypted?
	file.put(static_cast<std::underlying_type_t<Compression>>(comp));
}

bool FragmentStore::syncToStorage(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb) {
	if (!_reg.valid(fid)) {
		return false;
	}

	if (!_reg.all_of<Components::Ephemeral::FilePath>(fid)) {
		// not a file fragment?
		return false;
	}

	// split object storage

	MetaFileType meta_type = MetaFileType::TEXT_JSON; // TODO: better defaults
	if (_reg.all_of<Components::Ephemeral::MetaFileType>(fid)) {
		meta_type = _reg.get<Components::Ephemeral::MetaFileType>(fid).type;
	}

	Encryption meta_enc = Encryption::NONE; // TODO: better defaults
	Compression meta_comp = Compression::NONE; // TODO: better defaults

	if (meta_type != MetaFileType::TEXT_JSON) {
		if (_reg.all_of<Components::Ephemeral::MetaEncryptionType>(fid)) {
			meta_enc = _reg.get<Components::Ephemeral::MetaEncryptionType>(fid).enc;
		}

		if (_reg.all_of<Components::Ephemeral::MetaCompressionType>(fid)) {
			meta_comp = _reg.get<Components::Ephemeral::MetaCompressionType>(fid).comp;
		}
	} else {
		// we cant have encryption or compression

		// TODO: warning/error?

		// TODO: forcing for testing
		//if (_reg.all_of<Components::Ephemeral::MetaEncryptionType>(fid)) {
			_reg.emplace_or_replace<Components::Ephemeral::MetaEncryptionType>(fid, Encryption::NONE);
		//}
		//if (_reg.all_of<Components::Ephemeral::MetaCompressionType>(fid)) {
			_reg.emplace_or_replace<Components::Ephemeral::MetaCompressionType>(fid, Compression::NONE);
		//}
	}

	std::ofstream meta_file{
		_reg.get<Components::Ephemeral::FilePath>(fid).path + ".meta" + metaFileTypeSuffix(meta_type),
		std::ios::out | std::ios::trunc | std::ios::binary // always binary, also for text
	};

	if (!meta_file.is_open()) {
		return false;
	}

	std::ofstream data_file{
		_reg.get<Components::Ephemeral::FilePath>(fid).path,
		std::ios::out | std::ios::trunc | std::ios::binary // always binary, also for text
	};

	if (!data_file.is_open()) {
		return false;
	}

	// metadata type
	if (meta_type == MetaFileType::BINARY_MSGPACK) { // binary metadata file
		writeBinaryMetafileHeader(meta_file, meta_enc, meta_comp);
	}

	// sharing code between binary msgpack and text json for now
	nlohmann::json meta_data = nlohmann::json::object(); // metadata needs to be an object, null not allowed
	// metadata file

	for (const auto& [type_id, storage] : _reg.storage()) {
		if (!storage.contains(fid)) {
			continue;
		}

		std::cout << "storage type: type_id:" << type_id << " name:" << storage.type().name() << "\n";

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
		s_cb_it->second(storage.value(fid), meta_data[storage.type().name()]);
		//}
	}

	if (meta_type == MetaFileType::BINARY_MSGPACK) { // binary metadata file
		const auto res = nlohmann::json::to_msgpack(meta_data);
		meta_file.write(reinterpret_cast<const char*>(res.data()), res.size());
	} else if (meta_type == MetaFileType::TEXT_JSON) {
		meta_file << meta_data.dump(2, ' ', true);
	}

	// now data
	std::array<uint8_t, 1024> buffer;
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

		data_file.write(reinterpret_cast<const char*>(buffer.data()), buffer_actual_size);
	} while (buffer_actual_size == buffer.size());

	meta_file.flush();
	data_file.flush();

	// TODO: use temp files and move to old location

	if (_reg.all_of<Components::Ephemeral::DirtyTag>(fid)) {
		_reg.remove<Components::Ephemeral::DirtyTag>(fid);
	}

	return true;
}

static bool serl_json_data_enc_type(void* comp, nlohmann::json& out) {
	if (comp == nullptr) {
		return false;
	}

	auto& r_comp = *reinterpret_cast<Components::DataEncryptionType*>(comp);

	out = static_cast<std::underlying_type_t<Encryption>>(r_comp.enc);

	return true;
}

static bool serl_json_data_comp_type(void* comp, nlohmann::json& out) {
	if (comp == nullptr) {
		return false;
	}

	auto& r_comp = *reinterpret_cast<Components::DataCompressionType*>(comp);

	out = static_cast<std::underlying_type_t<Compression>>(r_comp.comp);

	return true;
}

void FragmentStore::registerSerializers(void) {
	_sc.registerSerializerJson<Components::DataEncryptionType>(serl_json_data_enc_type);
	_sc.registerSerializerJson<Components::DataCompressionType>(serl_json_data_comp_type);

	std::cout << "registered serl text json cbs:\n";
	for (const auto& [type_id, _] : _sc._serl_json) {
		std::cout << "  " << type_id << "\n";
	}
}

