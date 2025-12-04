#pragma once

#include <solanaceae/message3/registry_message_model.hpp>

#include "./texture_cache_defs.hpp"

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

struct ImageViewerPopup {
	MessageTextureCache& _mtc;

	Message3Handle _m{};
	float _scale {1.f};
	// keep track of full size from last frame
	uint32_t _width {0};
	uint32_t _height {0};

	bool _open_popup {false};

	//void reset(void);

	public:
		ImageViewerPopup(MessageTextureCache& mtc);

		// open popup with (image) message
		//void view(ObjectHandle o);
		void view(Message3Handle m);

		// call this each frame
		void render(float time_delta);

		// TODO: events (destroy/update)
};

