#pragma once

#include "./meta_components.hpp"
#include "./fragment_store_i.hpp"
#include "./fragment_store.hpp"

#include "./message_serializer.hpp"

#include <entt/entity/registry.hpp>
#include <entt/container/dense_map.hpp>
#include <entt/container/dense_set.hpp>

#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

#include <queue>
#include <vector>
#include <cstdint>

namespace Message::Components {

	using FUID = FragComp::ID;

	// unused
	struct FID {
		FragmentID fid {entt::null};
	};

	// points to the front/newer message
	// together they define a range that is,
	// eg the first(end) and last(begin) message being rendered
	// MFS requires there to be atleast one other fragment after/before,
	// if not loaded fragment with fitting tsrange(direction) available
	// uses fragmentAfter/Before()
	// they can exist standalone
	// if they are a pair, the inside is filled first
	// cursers require a timestamp ???
	struct ViewCurserBegin {
		Message3 curser_end{entt::null};
	};
	struct ViewCurserEnd {
		Message3 curser_begin{entt::null};
	};

	// TODO: add adjacency range comp or inside curser

	// TODO: unused
	// mfs will only load a limited number of fragments per tick (1),
	// so this tag will be set if we loaded a fragment and
	// every tick we check all cursers for this tag and continue
	// and remove once no fragment could be loaded anymore
	// (internal)
	struct TagCurserUnsatisfied {};

} // Message::Components

namespace Fragment::Components {
	struct MessagesTSRange {
		// timestamp range within the fragment
		uint64_t begin {0}; // newer msg -> higher number
		uint64_t end {0};
	};

	struct MessagesContact {
		std::vector<uint8_t> id;
	};

	// TODO: add src contact (self id)

} // Fragment::Components

// handles fragments for messages
// on new message: assign fuid
// on new and update: mark as fragment dirty
// on delete: mark as fragment dirty?
class MessageFragmentStore : public RegistryMessageModelEventI, public FragmentStoreEventI {
	protected:
		Contact3Registry& _cr;
		RegistryMessageModel& _rmm;
		FragmentStore& _fs;
		bool _fs_ignore_event {false};

		// for message components only
		MessageSerializerCallbacks _sc;

		void handleMessage(const Message3Handle& m);

		void loadFragment(Message3Registry& reg, FragmentHandle fh);

		struct SaveQueueEntry final {
			uint64_t ts_since_dirty{0};
			std::vector<uint8_t> id;
			Message3Registry* reg{nullptr};
		};
		std::queue<SaveQueueEntry> _fuid_save_queue;

		struct ECQueueEntry final {
			FragmentID fid;
			Contact3 c;
		};
		std::queue<ECQueueEntry> _event_check_queue;

		// range changed or fragment loaded.
		// we only load a limited number of fragments at once,
		// so we need to keep them dirty until nothing was loaded.
		entt::dense_set<Contact3> _potentially_dirty_contacts;

	public:
		MessageFragmentStore(
			Contact3Registry& cr,
			RegistryMessageModel& rmm,
			FragmentStore& fs
		);
		virtual ~MessageFragmentStore(void);

		MessageSerializerCallbacks& getMSC(void);

		float tick(float time_delta);

		void triggerScan(void);

	protected: // rmm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;

	protected: // fs
		bool onEvent(const Fragment::Events::FragmentConstruct& e) override;
};

