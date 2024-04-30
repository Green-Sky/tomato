#include "./object_store_ui.hpp"

#include <solanaceae/object_store/meta_components.hpp>

#include <imgui/imgui.h>

ObjectStoreUI::ObjectStoreUI(
	ObjectStore2& os
) : _os(os) {
	_ee.show_window = false;

	_ee.registerComponent<ObjectStore::Components::ID>("ID");
	_ee.registerComponent<ObjectStore::Components::DataCompressionType>("DataCompressionType");
}

void ObjectStoreUI::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("ObjectStore")) {
					if (ImGui::MenuItem("Inspector")) {
						//_show_add_friend_window = true;
						_ee.show_window = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
		}
		ImGui::End();
	}

	static Object selected_ent {entt::null};
	_ee.renderSimpleCombo(_os.registry(), selected_ent);
}

