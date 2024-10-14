#include "./contact_tree.hpp"

#include <solanaceae/contact/components.hpp>

#include "./contact_list.hpp"
#include "entt/entity/entity.hpp"

#include <imgui/imgui.h>

namespace Components {
	struct TagTreeViewOpen {};
};

ContactTreeWindow::ContactTreeWindow(Contact3Registry& cr, Theme& theme, ContactTextureCache& ctc) : _cr(cr), _theme(theme), _ctc(ctc) {
}

void ContactTreeWindow::render(void) {
	//{ // main window menubar injection
	//    // assumes the window "tomato" was rendered already by cg
	//    if (ImGui::Begin("tomato")) {
	//        if (ImGui::BeginMenuBar()) {
	//            ImGui::Separator();
	//            if (ImGui::BeginMenu("Settings")) {
	//                if (ImGui::MenuItem("settings window", nullptr, _show_window)) {
	//                    _show_window = !_show_window;
	//                }
	//                ImGui::EndMenu();
	//            }
	//            ImGui::EndMenuBar();
	//        }
	//    }
	//    ImGui::End();
	//}

	if (_show_window) {
		if (ImGui::Begin("ContactTreeWindow", &_show_window)) {

			if (ImGui::BeginTable("##table", 1, ImGuiTableFlags_None)) {
				// first we need all root nodes
				for (const auto [cv_root] : _cr.view<Contact::Components::TagRoot>().each()) {
					ImGui::TableNextRow();
					ImGui::PushID(entt::to_integral(cv_root));

					ImGui::TableNextColumn();

					Contact3Handle c_root {_cr, cv_root};
					bool open = c_root.all_of<Components::TagTreeViewOpen>();

					bool has_children = c_root.all_of<Contact::Components::ParentOf>();

					// roots are usually no normal contacts
					// they should display as a protocol or profile or account
					// TODO: set some table background instead of full span selected?
					if (renderContactBig(_theme, _ctc, c_root, 2, false, true)) {
						// clicked, toggle open
						if (open) {
							c_root.remove<Components::TagTreeViewOpen>();
							open = false;
						} else {
							c_root.emplace_or_replace<Components::TagTreeViewOpen>();
							open = true;
						}
					}

					if (open) {
						// render children

						ImGui::Indent();

						if (c_root.all_of<Contact::Components::ParentOf>()) {
							for (const auto cv_child : c_root.get<Contact::Components::ParentOf>().subs) {
								ImGui::PushID(entt::to_integral(cv_child));

								Contact3Handle c_child {_cr, cv_child};
								renderContactBig(_theme, _ctc, c_child, 2);

								ImGui::PopID();
							}
						} else {
							// TODO: remove
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextDisabled("no parent of");
						}

						ImGui::Unindent();

					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}

