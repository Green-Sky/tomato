#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include "./frame_streams/stream_manager.hpp"

class StreamManagerUI {
	ObjectStore2& _os;
	StreamManager& _sm;

	bool _show_window {false};

	public:
		StreamManagerUI(ObjectStore2& os, StreamManager& sm);

		void render(void);
};

