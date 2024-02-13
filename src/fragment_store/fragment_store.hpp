#pragma once

#include "./fragment_store_i.hpp"

#include "./types.hpp"
#include "./meta_components.hpp"

#include "./serializer.hpp"

#include <entt/core/type_info.hpp>
#include <entt/entity/registry.hpp>

#include <nlohmann/json_fwd.hpp>

#include <vector>
#include <array>
#include <cstdint>
#include <random>

struct FragmentStore : public FragmentStoreI {
	using FragmentHandle = entt::basic_handle<entt::basic_registry<FragmentID>>;

	entt::basic_registry<FragmentID> _reg;

	std::minstd_rand _rng{std::random_device{}()};
	std::array<uint8_t, 8> _session_uuid_namespace;

	std::string _default_store_path;

	uint64_t _memory_budget {10u*1024u*1024u};
	uint64_t _memory_usage {0u};

	SerializerCallbacks _sc;

	FragmentStore(void);
	FragmentStore(std::array<uint8_t, 8> session_uuid_namespace);

	// HACK: get access to the reg
	FragmentHandle fragmentHandle(FragmentID fid);

	// TODO: make the frags ref counted

	// TODO: check for exising
	std::vector<uint8_t> generateNewUID(std::array<uint8_t, 8>& uuid_namespace);
	std::vector<uint8_t> generateNewUID(void);

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
	// unload?

	// ========== sync fragment to storage ==========
	using write_to_storage_fetch_data_cb = uint64_t(uint8_t* request_buffer, uint64_t buffer_size);
	// calls data_cb with a buffer to be filled in, cb returns actual count of data. if returned < max, its the last buffer.
	bool syncToStorage(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);
	bool syncToStorage(FragmentID fid, const uint8_t* data, const uint64_t data_size);

	// fragment discovery?

	private:
		void registerSerializers(void); // internal comps
		// internal actual backends
		// TODO: seperate out
		bool syncToMemory(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);
		bool syncToFile(FragmentID fid, std::function<write_to_storage_fetch_data_cb>& data_cb);
};

