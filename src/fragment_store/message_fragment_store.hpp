#pragma once

#include "./meta_components.hpp"
#include "./fragment_store_i.hpp"
#include "./fragment_store.hpp"

#include <entt/entity/registry.hpp>
#include <entt/container/dense_map.hpp>

#include <solanaceae/message3/registry_message_model.hpp>

#include <queue>
#include <vector>
#include <cstdint>

namespace Message::Components {

	using FUID = FragComp::ID;

	struct FID {
		FragmentID fid {entt::null};
	};

} // Message::Components

namespace Fragment::Components {
	struct MessagesTSRange {
		// timestamp range within the fragment
		uint64_t begin {0};
		uint64_t end {0};
	};
} // Fragment::Components

// handles fragments for messages
// on new message: assign fuid
// on new and update: mark as fragment dirty
// on delete: mark as fragment dirty?
class MessageFragmentStore : public RegistryMessageModelEventI {
	protected:
		RegistryMessageModel& _rmm;
		FragmentStore& _fs;

		void handleMessage(const Message3Handle& m);

		struct OpenFrag final {
			uint64_t ts_begin {0};
			uint64_t ts_end {0};
			std::vector<uint8_t> uid;
		};
		// only contains fragments with <1024 messages and <28h tsrage
		// TODO: this needs to move into the message reg
		std::vector<OpenFrag> _fuid_open;

		struct QueueEntry final {
			uint64_t ts_since_dirty{0};
			std::vector<uint8_t> id;
		};
		std::queue<QueueEntry> _fuid_save_queue;

	public:
		MessageFragmentStore(
			RegistryMessageModel& rmm,
			FragmentStore& fs
		);
		virtual ~MessageFragmentStore(void);

		float tick(float time_delta);

	protected: // rmm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;
};

