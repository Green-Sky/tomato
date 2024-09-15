#pragma once

#include <cstdint>
#include <solanaceae/object_store/fwd.hpp>
#include "./stream_manager.hpp"
#include "./texture_uploader.hpp"

// provides a sink and a small window displaying a SDLVideoFrame
class DebugVideoTap {
	ObjectStore2& _os;
	StreamManager& _sm;
	TextureUploaderI& _tu;

	ObjectHandle _selected_src;
	ObjectHandle _tap;

	uint64_t _tex {0};
	uint32_t _tex_w {0};
	uint32_t _tex_h {0};

	uint64_t _v_last_ts {0}; // ns
	float _v_interval_avg {0.f}; // s

	public:
		DebugVideoTap(ObjectStore2& os, StreamManager& sm, TextureUploaderI& tu);
		~DebugVideoTap(void);

		float render(void);

		void switchTo(ObjectHandle o);
};

