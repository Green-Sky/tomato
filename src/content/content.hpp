#pragma once

#include <entt/container/dense_set.hpp>

#include <solanaceae/object_store/object_store.hpp>
#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/message3/registry_message_model.hpp>

#include <solanaceae/file/file2.hpp>

namespace Content1::Components {

	// TODO: design it as a tree?

	// or something
	struct TagFile {};
	struct TagAudioStream {};
	struct TagVideoStream {};

	struct TimingTiedTo {
		entt::dense_set<ObjectHandle> ties;
	};

	// the associated messages, if any
	// useful if you want to update progress on the message
	struct Messages {
		std::vector<Message3Handle> messages;
	};

	// ?
	struct SuspectedParticipants {
		entt::dense_set<Contact3> participants;
	};

	struct ReadHeadHint {
		// points to the first byte we want
		// this is just a hint, that can be set from outside
		// to guide the sequential "piece picker" strategy
		// the strategy *should* set this to the first byte we dont yet have
		uint64_t offset_into_file {0u};
	};

} // Content::Components

// TODO: i have no idea
struct RawFile2ReadFromContentFactoryI {
	virtual std::shared_ptr<File2I> open(ObjectHandle h) = 0;
};

