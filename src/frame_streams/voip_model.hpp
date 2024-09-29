#pragma once

#include <solanaceae/contact/contact_model3.hpp>
#include <solanaceae/object_store/fwd.hpp>

struct VoIPModelI;

namespace Components::VoIP {

	struct TagVoIPSession {};

	// to talk to the model handling this session
	//struct VoIPModel {
		//VoIPModelI* ptr {nullptr};
	//};

	struct SessionState {
		// ????
		// incoming
		// outgoing
		enum class State {
			RINGING,
			CONNECTED,
		} state;
	};

	struct SessionContact {
		Contact3 c{entt::null};
	};

	struct StreamSources {
		// list of all stream sources originating from this VoIP session
		std::vector<Object> streams;
	};

	struct StreamSinks {
		// list of all stream sinks going to this VoIP session
		std::vector<Object> streams;
	};

} // Components::VoIP

// TODO: events? piggyback on objects?

// stream model instead?? -> more generic than "just" audio and video?
// or specialized like this
// streams abstract type in a nice way
struct VoIPModelI {
	virtual ~VoIPModelI(void) {}

	// enters a call/voicechat/videocall ???
	// - contact
	// - default stream sources/sinks ?
	// - enable a/v ? -> done on connecting to sources
	// returns object tieing together the VoIP session
	struct DefaultConfig {
		bool incoming_audio {true};
		bool incoming_video {true};
		bool outgoing_audio {true};
		bool outgoing_video {true};
	};
	virtual ObjectHandle enter(const Contact3 c, const DefaultConfig& defaults = {true, true, true, true}) { return {}; }

	// accept/join an invite to a session
	virtual bool accept(ObjectHandle session) { return false; }

	// leaves a call
	// - VoIP session object
	virtual bool leave(ObjectHandle session) { return false; }
};

