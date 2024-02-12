#pragma once

#include "./fragment_store_i.hpp"
#include "entt/entity/fwd.hpp"

#include <entt/core/fwd.hpp>
#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>
#include <entt/container/dense_map.hpp>

#include <nlohmann/json_fwd.hpp>

#include <utility>
#include <vector>
#include <array>
#include <cstdint>

enum class Encryption : uint8_t {
	NONE = 0x00,
};
enum class Compression : uint8_t {
	NONE = 0x00,
};
enum class MetaFileType : uint8_t {
	TEXT_JSON,
	//BINARY_ARB,
	BINARY_MSGPACK,
};

namespace Components {

	// TODO: is this special and should this be saved to meta or not (its already in the file name on disk)
	struct ID {
		std::vector<uint8_t> v;
	};

	struct DataEncryptionType {
		Encryption enc {Encryption::NONE};
	};

	struct DataCompressionType {
		Compression comp {Compression::NONE};
	};


	// meta that is not written to (meta-)file
	namespace Ephemeral {

		// excluded from file meta
		struct FilePath {
			// contains store path, if any
			std::string path;
		};

		// TODO: seperate into remote and local?
		// (remote meaning eg. the file on disk was changed by another program)
		struct DirtyTag {};


		// type as comp
		struct MetaFileType {
			::MetaFileType type {::MetaFileType::TEXT_JSON};
		};

		struct MetaEncryptionType {
			Encryption enc {Encryption::NONE};
		};

		struct MetaCompressionType {
			Compression comp {Compression::NONE};
		};

	} // Ephemeral

} // Components

struct SerializerCallbacks {
	// nlohmann
	// json/msgpack
	using serialize_json_fn = bool(*)(void* comp, nlohmann::json& out);
	entt::dense_map<entt::id_type, serialize_json_fn> _serl_json;

	using deserialize_json_fn = bool(*)(void* comp, const nlohmann::json& in);
	entt::dense_map<entt::id_type, deserialize_json_fn> _deserl_json;

	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info) {
		_serl_json[type_info.hash()] = fn;
	}
	template<typename CompType>
	void registerSerializerJson(serialize_json_fn fn, const entt::type_info& type_info = entt::type_id<CompType>()) { registerSerializerJson(fn, type_info); }

	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info) {
		_deserl_json[type_info.hash()] = fn;
	}
	template<typename CompType>
	void registerDeSerializerJson(deserialize_json_fn fn, const entt::type_info& type_info = entt::type_id<CompType>()) { registerDeSerializerJson(fn, type_info); }
};

struct FragmentStore : public FragmentStoreI {
	entt::basic_registry<FragmentID> _reg;

	std::array<uint8_t, 8> _session_uuid_namespace;

	std::string _default_store_path;

	uint64_t _memory_budget {10u*1024u*1024u};
	uint64_t _memory_usage {0u};

	SerializerCallbacks _sc;

	FragmentStore(void);
	FragmentStore(std::array<uint8_t, 8> session_uuid_namespace);

	// HACK: get access to the reg
	entt::basic_handle<entt::basic_registry<FragmentID>> fragmentHandle(FragmentID fid);

	// TODO: make the frags ref counted

	// ========== new fragment ==========

	// memory backed owned
	FragmentID newFragmentMemoryOwned(
		const std::vector<uint8_t>& id,
		size_t initial_size
	);

	// memory backed view (can only be added? not new?)

	// file backed (rw...)
	// needs to know which store path to put into
	FragmentID newFragmentFile(
		std::string_view store_path,
		MetaFileType mft,
		const std::vector<uint8_t>& id
	);
	// this variant generate a new, mostly unique, id for us
	FragmentID newFragmentFile(
		std::string_view store_path,
		MetaFileType mft
	);

	// ========== add fragment ==========

	// ========== get fragment ==========
	FragmentID getFragmentByID(
		const std::vector<uint8_t>& id
	);
	FragmentID getFragmentCustomMatcher(
		std::function<bool(FragmentID)>& fn
	);

	// remove fragment?

	// syncs fragment to file

	using write_to_storage_fetch_data_cb = uint64_t(uint8_t* request_buffer, uint64_t buffer_size);
	// calls data_cb with a buffer to be filled in, cb returns actual count of data. if returned < max, its the last buffer.
	bool syncToStorage(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);

	// unload frags?
	// if frags are file backed, we can close the file if not needed

	// fragment discovery?

	private:
		void registerSerializers(void); // internal comps
		// internal actual backends
		bool syncToMemory(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);
		bool syncToFile(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);
};

