#include "./start_screen.hpp"

#include "./main_screen.hpp"

#include "./json_to_config.hpp"

#include <nlohmann/json.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <cctype>
#include <memory>
#include <filesystem>
#include <fstream>

StartScreen::StartScreen(const std::vector<std::string_view>& args, SDL_Renderer* renderer, Theme& theme) : _renderer(renderer), _theme(theme) {
	for (size_t ai = 1; ai < args.size(); ai++) {
		if (args.at(ai) == "--config" || args.at(ai) == "-c") {
			if (args.size() == ai+1) {
				std::cerr << "TOMATO error: argument '" << args.at(ai) << "' missing parameter!\n";
				break;
			}
			ai++;

			const auto& config_path = args.at(ai);
			auto config_file = std::ifstream(static_cast<std::string>(config_path));
			if (!config_file.is_open()) {
				std::cerr << "TOMATO error: failed to open config file '" << config_path << "'\n";
				break;
			}

			auto config_json = nlohmann::ordered_json::parse(config_file);
			if (!load_json_into_config(config_json, _conf)) {
				std::cerr << "TOMATO error in config json, exiting...\n";
				break;
			}
		} else if (args.at(ai) == "--plugin" || args.at(ai) == "-p") {
			if (args.size() == ai+1) {
				std::cerr << "TOMATO error: argument '" << args.at(ai) << "' missing parameter!\n";
				break;
			}
			ai++;

			const auto& plugin_path = args.at(ai);
			// TODO: check for dups
			queued_plugin_paths.push_back(static_cast<std::string>(plugin_path));
		} else {
			std::cerr << "TOMATO error: unknown cli arg: '" << args.at(ai) << "'\n";
		}
	}
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
			auto new_screen = std::make_unique<MainScreen>(std::move(_conf), _renderer, _theme, _tox_profile_path, _password, _user_name, queued_plugin_paths);
			return new_screen.release();
		}
	}

	_fss.render();

	return nullptr;
}

Screen* StartScreen::tick(float, bool&) {
	return nullptr;
}

