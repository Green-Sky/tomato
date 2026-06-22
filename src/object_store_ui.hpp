#pragma once

#include <solanaceae/object_store/object_store.hpp>

#include "./imgui_entt_entity_editor.hpp"
#include "./chat_gui/theme.hpp"

class ObjectStoreUI {
	ObjectStore2& _os;
	Theme& _theme;

	MM::EntityEditor<Object> _ee;

	public:
	ObjectStoreUI(
			ObjectStore2& os,
			Theme& theme
		);

		void render(void);
};

