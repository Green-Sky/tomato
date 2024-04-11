#pragma once

#include "./meta_components.hpp"
#include "./object_store.hpp"

#include "./uuid_generator.hpp"

#include "./message_serializer.hpp"

#include "./messages_meta_components.hpp"

#include <entt/container/dense_map.hpp>
#include <entt/container/dense_set.hpp>

#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

#include <deque>
#include <vector>
#include <cstdint>

namespace Message::Components {

	// unused, consumes too much memory (highly compressable)
	//using FUID = FragComp::ID;

	struct Obj {
		Object o {entt::null};
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

// handles fragments for messages
// on new message: assign fuid
// on new and update: mark as fragment dirty
// on delete: mark as fragment dirty?
class MessageFragmentStore : public RegistryMessageModelEventI, public ObjectStoreEventI {
	protected:
		Contact3Registry& _cr;
		RegistryMessageModel& _rmm;
		ObjectStore2& _os;
		StorageBackendI& _sb;
		bool _fs_ignore_event {false};

		UUIDGenerator_128_128 _session_uuid_gen;

		// for message components only
		MessageSerializerCallbacks _sc;

		void handleMessage(const Message3Handle& m);

		void loadFragment(Message3Registry& reg, ObjectHandle oh);

		bool syncFragToStorage(ObjectHandle oh, Message3Registry& reg);

		struct SaveQueueEntry final {
			uint64_t ts_since_dirty{0};
			//std::vector<uint8_t> id;
			ObjectHandle id;
			Message3Registry* reg{nullptr};
		};
		std::deque<SaveQueueEntry> _fuid_save_queue;

		struct ECQueueEntry final {
			ObjectHandle fid;
			Contact3 c;
		};
		std::deque<ECQueueEntry> _event_check_queue;

		// range changed or fragment loaded.
		// we only load a limited number of fragments at once,
		// so we need to keep them dirty until nothing was loaded.
		entt::dense_set<Contact3> _potentially_dirty_contacts;

	public:
		MessageFragmentStore(
			Contact3Registry& cr,
			RegistryMessageModel& rmm,
			ObjectStore2& os,
			StorageBackendI& sb
		);
		virtual ~MessageFragmentStore(void);

		MessageSerializerCallbacks& getMSC(void);

		float tick(float time_delta);

	protected: // rmm
		bool onEvent(const Message::Events::MessageConstruct& e) override;
		bool onEvent(const Message::Events::MessageUpdated& e) override;

	protected: // fs
		bool onEvent(const ObjectStore::Events::ObjectConstruct& e) override;
		bool onEvent(const ObjectStore::Events::ObjectUpdate& e) override;
};

