#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include <solanaceae/contact/fwd.hpp>
#include "../frame_streams/stream_manager.hpp"
#include "../texture_uploader.hpp"

namespace Contact::Components {
	struct VoIPVideoTexture {
		uint64_t texture {0};
		// w,h,format
		size_t frames_since_render {0};
	};
} // Contact::Components

// creates/updates/destroys contact voip video sink textures that where rendered
// also connects/disconnects to voip stream source
// renders standalone(pop-out) video windows
class VoIPVideoDisplay final {
	ObjectStore2& _os;
	ContactStore4I& _cs;
	StreamManager& _sm;
	TextureUploaderI& _tu;

	const size_t _frames_till_destory {2};

	public:
		VoIPVideoDisplay(
			ObjectStore2& os,
			ContactStore4I& cs,
			StreamManager& sm,
			TextureUploaderI& tu
		);
		~VoIPVideoDisplay(void);

	public:
		// call every frame
		float render(float delta_time);
};
