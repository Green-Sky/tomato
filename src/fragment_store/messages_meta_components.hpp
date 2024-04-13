#pragma once

#include <solanaceae/object_store/meta_components.hpp>

namespace ObjectStore::Components {
	struct MessagesVersion {
		// messages Object version
		// 1 -> text_json
		// 2 -> msgpack
		uint16_t v {2};
	};

	struct MessagesTSRange {
		// timestamp range within the fragment
		uint64_t begin {0}; // newer msg -> higher number
		uint64_t end {0};
	};

	struct MessagesContact {
		std::vector<uint8_t> id;
	};

	// TODO: add src contact (self id)

} // ObjectStore::Components

// old
namespace Fragment::Components {
	struct MessagesTSRange : public ObjComp::MessagesTSRange {};
	struct MessagesContact : public ObjComp::MessagesContact {};
} // Fragment::Components

#include "./messages_meta_components_id.inl"

