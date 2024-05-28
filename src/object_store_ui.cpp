#include "./object_store_ui.hpp"

#include <solanaceae/object_store/meta_components.hpp>

#include <imgui/imgui.h>

#include <solanaceae/message3/components.hpp>

ObjectStoreUI::ObjectStoreUI(
	ObjectStore2& os
) : _os(os) {
	_ee.show_window = false;

	_ee.registerComponent<ObjectStore::Components::ID>("ID");
	_ee.registerComponent<ObjectStore::Components::DataCompressionType>("DataCompressionType");

	_ee.registerComponent<Message::Components::Transfer::FileInfo>("Transfer::FileInfo");
	_ee.registerComponent<Message::Components::Transfer::FileInfoLocal>("Transfer::FileInfoLocal");
}

void ObjectStoreUI::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("ObjectStore")) {
					if (ImGui::MenuItem("Inspector")) {
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

