#include "./object_store_ui.hpp"

#include <solanaceae/util/utils.hpp>
#include <solanaceae/object_store/meta_components.hpp>
#include <solanaceae/object_store/meta_components_file.hpp>
#include "./os_comps.hpp"

#include <imgui/imgui.h>

namespace MM {

template<> void ComponentEditorWidget<ObjectStore::Components::ID>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<ObjectStore::Components::ID>(entity);

	const auto str = bin2hex(c.v);
	if (ImGui::SmallButton("copy")) {
		ImGui::SetClipboardText(str.c_str());
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(str.c_str());
}

#if 0
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

#endif

template<> void ComponentEditorWidget<ObjComp::F::SingleInfo>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<ObjComp::F::SingleInfo>(entity);
	if (!c.file_name.empty()) {
		ImGui::Text("file name: %s", c.file_name.c_str());
	}

	ImGui::Text("file size: %lu", c.file_size);
}

template<> void ComponentEditorWidget<ObjComp::F::SingleInfoLocal>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<ObjComp::F::SingleInfoLocal>(entity);
	if (!c.file_path.empty()) {
		ImGui::Text("file path: %s", c.file_path.c_str());
	}
}

template<> void ComponentEditorWidget<ObjComp::Ephemeral::File::TransferStats>(entt::basic_registry<Object>& registry, Object entity) {
	auto& c = registry.get<ObjComp::Ephemeral::File::TransferStats>(entity);
	ImGui::Text("upload rate  : %.1f bytes/s", c.rate_up);
	ImGui::Text("download rate: %.1f bytes/s", c.rate_down);
	ImGui::Text("total bytes uploaded  : %lu bytes", c.total_up);
	ImGui::Text("total bytes downloaded: %lu bytes", c.total_down);
}

} // MM

ObjectStoreUI::ObjectStoreUI(
	ObjectStore2& os
) : _os(os) {
	_ee.show_window = false;

	_ee.registerComponent<ObjectStore::Components::ID>("ID");
	_ee.registerComponent<ObjectStore::Components::DataCompressionType>("DataCompressionType");

	_ee.registerComponent<ObjComp::Ephemeral::FilePath>("Ephemeral::FilePath");

	_ee.registerComponent<ObjComp::F::SingleInfo>("File::SingleInfo");
	_ee.registerComponent<ObjComp::F::SingleInfoLocal>("File::SingleInfoLocal");
	_ee.registerComponent<ObjComp::F::Collection>("File::Collection");
	_ee.registerComponent<ObjComp::F::CollectionInfo>("File::CollectionInfo");
	_ee.registerComponent<ObjComp::F::CollectionInfoLocal>("File::CollectionInfoLocal");
	_ee.registerComponent<ObjComp::F::LocalHaveBitset>("File::LocalHaveBitset");
	_ee.registerComponent<ObjComp::F::RemoteHaveBitset>("File::RemoteHaveBitset");

	_ee.registerComponent<ObjComp::Ephemeral::File::DownloadPriority>("Ephemeral::File::DownloadPriority");
	_ee.registerComponent<ObjComp::Ephemeral::File::ReadHeadHint>("Ephemeral::File::ReadHeadHint");
	_ee.registerComponent<ObjComp::Ephemeral::File::TransferStats>("Ephemeral::File::TransferStats");
	_ee.registerComponent<ObjComp::Ephemeral::File::TransferStatsSeparated>("Ephemeral::File::TransferStatsSeparated");
}

void ObjectStoreUI::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("ObjectStore")) {
					if (ImGui::MenuItem("Inspector", nullptr, _ee.show_window)) {
						_ee.show_window = !_ee.show_window;
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

