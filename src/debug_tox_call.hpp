#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include "./tox_av.hpp"
#include "./texture_uploader.hpp"

#include <map>
#include <cstdint>

class DebugToxCall : public ToxAVEventI {
	ObjectStore2& _os;
	ToxAV& _toxav;
	TextureUploaderI& _tu;

	struct Call {
		bool incoming {false};
		bool incoming_a {false};
		bool incoming_v {false};

		uint32_t state {0}; // ? just last state ?

		uint32_t abr {0};
		uint32_t vbr {0};

		size_t num_a_frames {0};
		size_t num_v_frames {0};

		// fps moving interval
		uint64_t last_v_frame_timepoint {0};
		float v_frame_interval_avg {0.f};

		uint64_t last_v_frame_tex {0};
		uint64_t last_v_frame_width {0};
		uint64_t last_v_frame_height {0};
	};
	// tox friend id -> call
	std::map<uint32_t, Call> _calls;

	public:
		DebugToxCall(ObjectStore2& os, ToxAV& toxav, TextureUploaderI& tu);
		~DebugToxCall(void) {}

		void tick(float time_delta);
		float render(void);

	protected: // toxav events
		bool onEvent(const Events::FriendCall&) override;
		bool onEvent(const Events::FriendCallState&) override;
		bool onEvent(const Events::FriendAudioBitrate&) override;
		bool onEvent(const Events::FriendVideoBitrate&) override;
		bool onEvent(const Events::FriendAudioFrame&) override;
		bool onEvent(const Events::FriendVideoFrame&) override;
};
