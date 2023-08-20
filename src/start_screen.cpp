#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include <imgui/imgui.h>

#include <memory>
#include <filesystem>

StartScreen::StartScreen(SDL_Renderer* renderer) : _renderer(renderer) {
}

Screen* StartScreen::poll(bool&) {

	// TODO: imgui tox profile selector?

	// +----------------------------
	// | |*tox profile*| plugins |
	// | +------+ +--------
	// | | ICON | | fileselector/dropdown?
	// | |      | | password input
	// | +------+ +--------
	// +----------------------------

	if (ImGui::BeginTabBar("view")) {
		if (ImGui::BeginTabItem("load profile")) {
			if (ImGui::Button("select")) {
				_fss.requestFile(
					[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
					[this](const auto& path) {
						tox_profile_path = path.string();
					},
					[](){}
				);
			}
			ImGui::SameLine();
			ImGui::Text("profile: %s", tox_profile_path.c_str());

			ImGui::Text("TODO: profile password");

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("create profile")) {
			ImGui::Text("TODO: profile path");
			ImGui::Text("TODO: profile name");
			ImGui::Text("TODO: profile password");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("plugins")) {
			// list of selected plugins (in order)
			for (auto it = queued_plugin_paths.begin(); it != queued_plugin_paths.end();) {
				ImGui::PushID(it->c_str());
				if (ImGui::SmallButton("-")) {
					it = queued_plugin_paths.erase(it);
					ImGui::PopID();
					continue;
				}
				ImGui::SameLine();

				ImGui::TextUnformatted(it->c_str());

				ImGui::PopID();
				it++;
			}

			if (ImGui::Button("+")) {
				_fss.requestFile(
					[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
					[this](const auto& path) {
						queued_plugin_paths.push_back(path.string());
					},
					[](){}
				);
			}

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Separator();

	if (ImGui::Button("load", {60, 25})) {
		auto new_screen = std::make_unique<MainScreen>(_renderer, tox_profile_path, queued_plugin_paths);
		return new_screen.release();
	}

	_fss.render();

	return nullptr;
}

