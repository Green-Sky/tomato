#include "./layout_strategy.hpp"

#include "../chat_gui4.hpp"

#include <imgui.h>

void DesktopLayout::render(ChatGui4& gui, const float time_delta, const bool window_focused) {
	if (gui._contact_stack.empty()) {
		gui.renderContactList(gui.TEXT_BASE_WIDTH*60);
	} else {
		gui.renderContactList(gui.TEXT_BASE_WIDTH*35);
	}

	if (gui._contact_list_sortable) {
		gui._cls.sort();
	}

	if (!gui._contact_stack.empty()) {
		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused)) {
			gui._contact_stack.pop();
		} else {
			ImGui::SameLine();
			gui._contact_stack.top()->render(window_focused, time_delta, true, ImGuiChildFlags_Borders);
		}
	}
}

void SinglePlaneLayout::render(ChatGui4& gui, const float time_delta, const bool window_focused) {
	if (gui._contact_stack.empty()) {
		gui.renderContactList();
		if (gui._contact_list_sortable) {
			gui._cls.sort();
		}
	} else {
		if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteFocused)) {
			gui._contact_stack.pop();
			return;
		}

		gui._contact_stack.top()->render(window_focused, time_delta, true, ImGuiChildFlags_None, true);
	}
}
