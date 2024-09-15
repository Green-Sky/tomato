#pragma once

#include <solanaceae/object_store/fwd.hpp>
#include "./stream_manager.hpp"

class StreamManagerUI {
	ObjectStore2& _os;
	StreamManager& _sm;

	public:
		StreamManagerUI(ObjectStore2& os, StreamManager& sm);

		void render(void);
};

