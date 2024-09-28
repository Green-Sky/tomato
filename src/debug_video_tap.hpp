#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include "./frame_streams/stream_manager.hpp"
#include "./texture_uploader.hpp"

// provides a sink and a small window displaying a SDLVideoFrame
// HACK: provides a test video source
class DebugVideoTap {
	ObjectStore2& _os;
	StreamManager& _sm;
	TextureUploaderI& _tu;

	ObjectHandle _tap;
	ObjectHandle _src;

	public:
		DebugVideoTap(ObjectStore2& os, StreamManager& sm, TextureUploaderI& tu);
		~DebugVideoTap(void);

		float render(void);
};

