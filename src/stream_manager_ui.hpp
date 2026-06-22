#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include "./chat_gui/theme.hpp"
#include "./frame_streams/stream_manager.hpp"

class StreamManagerUI {
	ObjectStore2& _os;
	StreamManager& _sm;
	Theme& _theme;

	bool _show_window {false};

	public:
		StreamManagerUI(ObjectStore2& os, StreamManager& sm, Theme& theme);

		void render(void);
};

