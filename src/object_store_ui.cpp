#include "./object_store_ui.hpp"

#include <solanaceae/util/utils.hpp>
#include <solanaceae/object_store/meta_components.hpp>

#include <imgui/imgui.h>

#include <solanaceae/message3/components.hpp>

namespace MM {

template<> void ComponentEditorWidget<ObjectStore::Components::ID>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<ObjectStore::Components::ID>(entity);

	const auto str = bin2hex(c.v);
	ImGui::TextUnformatted(str.c_str());
}

template<> void ComponentEditorWidget<Message::Components::Transfer::FileInfo>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<Message::Components::Transfer::FileInfo>(entity);

	ImGui::Text("Total size: %lu", c.total_size);
	if (ImGui::BeginTable("file list", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();

		for (const auto& entry : c.file_list) {
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(entry.file_name.c_str());

			ImGui::TableNextColumn();
			ImGui::Text("%lu", entry.file_size);
		}

		ImGui::EndTable();
	}
}

template<> void ComponentEditorWidget<Message::Components::Transfer::FileInfoLocal>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<Message::Components::Transfer::FileInfoLocal>(entity);

	if (ImGui::BeginTable("file list", 1, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit)) {
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
		//ImGui::TableSetupColumn("size", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();

		for (const auto& entry : c.file_list) {
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(entry.c_str());
		}

		ImGui::EndTable();
	}
}

template<> void ComponentEditorWidget<Message::Components::Transfer::BytesReceived>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<Message::Components::Transfer::BytesReceived>(entity);
	ImGui::Text("total bytes received: %lu", c.total);
}

template<> void ComponentEditorWidget<Message::Components::Transfer::BytesSent>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<Message::Components::Transfer::BytesSent>(entity);
	ImGui::Text("total bytes sent: %lu", c.total);
}

} // MM

ObjectStoreUI::ObjectStoreUI(
	ObjectStore2& os
) : _os(os) {
	_ee.show_window = false;

	_ee.registerComponent<ObjectStore::Components::ID>("ID");
	_ee.registerComponent<ObjectStore::Components::DataCompressionType>("DataCompressionType");

	_ee.registerComponent<Message::Components::Transfer::FileInfo>("Transfer::FileInfo");
	_ee.registerComponent<Message::Components::Transfer::FileInfoLocal>("Transfer::FileInfoLocal");
	_ee.registerComponent<Message::Components::Transfer::BytesReceived>("Transfer::BytesReceived");
	_ee.registerComponent<Message::Components::Transfer::BytesSent>("Transfer::BytesSent");
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

