#include "./settings_window.hpp"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

SettingsWindow::SettingsWindow(ConfigModelI& conf) : _conf(conf) {
}

void SettingsWindow::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("Settings")) {
					if (ImGui::MenuItem("show settings window")) {
						_show_window = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}

	if (_show_window) {
		if (ImGui::Begin("Settings", &_show_window)) {
			ImGui::Text("Settings here sldjflsadfs");
		}
		ImGui::End();
	}
}

