#pragma once

#include <solanaceae/object_store/object_store.hpp>

#include "./imgui_entt_entity_editor.hpp"

class ObjectStoreUI {
	ObjectStore2& _os;

	MM::EntityEditor<Object> _ee;

	public:
		ObjectStoreUI(
			ObjectStore2& os
		);

		void render(void);
};

