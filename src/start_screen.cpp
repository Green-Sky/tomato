#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <cctype>
#include <memory>
#include <filesystem>

StartScreen::StartScreen(SDL_Renderer* renderer, Theme& theme) : _renderer(renderer), _theme(theme) {
}

Screen* StartScreen::render(float, bool&) {

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
			_new_save = false;

			ImGui::TextUnformatted("profile :");
			ImGui::SameLine();
			if (ImGui::Button("select")) {
				_fss.requestFile(
					[](const auto& path) -> bool { return std::filesystem::is_regular_file(path); },
					[this](const auto& path) {
						_tox_profile_path = path.string();
					},
					[](){}
				);
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(_tox_profile_path.c_str());

			ImGui::TextUnformatted("password:");
			ImGui::SameLine();
			if (_show_password) {
				ImGui::InputText("##password", &_password);
			} else {
				ImGui::InputText("##password", &_password, ImGuiInputTextFlags_Password);
			}
			ImGui::SameLine();
			ImGui::Checkbox("show password", &_show_password);

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("create profile")) {
			_new_save = true;

			ImGui::TextUnformatted("username:");
			ImGui::SameLine();
			if (ImGui::InputText("##user_name", &_user_name)) {
				std::string tmp_copy = _user_name;
				for (auto& c : tmp_copy) {
					if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '.') {
						c = '_';
					}
				}

				if (tmp_copy.empty()) {
					tmp_copy = "unnamed-tomato";
				}

				_tox_profile_path = tmp_copy + ".tox";
			}

			ImGui::TextUnformatted("password:");
			ImGui::SameLine();
			if (_show_password) {
				ImGui::InputText("##password", &_password);
			} else {
				ImGui::InputText("##password", &_password, ImGuiInputTextFlags_Password);
			}
			ImGui::SameLine();
			ImGui::Checkbox("show password", &_show_password);

			ImGui::TextUnformatted("TODO: profile path (current path for now)");

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

	if (!_new_save && !std::filesystem::is_regular_file(_tox_profile_path)) {
		// load but file missing

		ImGui::BeginDisabled();
		ImGui::Button("load", {60, 25});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file does not exist");
		}
	} else if (_new_save && std::filesystem::exists(_tox_profile_path)) {
		// new but file exists

		ImGui::BeginDisabled();
		ImGui::Button("load", {60, 25});
		ImGui::EndDisabled();

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("file already exists");
		}
	} else {
		if (ImGui::Button("load", {60, 25})) {
			auto new_screen = std::make_unique<MainScreen>(_renderer, _theme, _tox_profile_path, _password, _user_name, queued_plugin_paths);
			return new_screen.release();
		}
	}

	_fss.render();

	return nullptr;
}

Screen* StartScreen::tick(float, bool&) {
	return nullptr;
}

