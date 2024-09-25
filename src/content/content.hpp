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

} // Content::Components

