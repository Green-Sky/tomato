#include "./settings_window.hpp"

#include <solanaceae/util/simple_config_model.hpp>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <tuple>

#include <iostream>

template<typename FNCat, typename ...MapTypes>
void iterateMultMaps(FNCat&& fn_cat, MapTypes& ...map_types) {
	// the iterators
	std::tuple its{map_types.begin()...};

	// assuming ever maps returned .begin() has a unique type
	// TODO: use index instead
	while ( ((std::get<decltype(map_types.begin())>(its) != map_types.end()) || ...) ) {
		// figure out current module and participants

		std::string_view current_module;
		// find current_module
		((
			current_module = (
				std::get<decltype(map_types.begin())>(its) != map_types.end() && (current_module.empty() || current_module >= std::get<decltype(map_types.begin())>(its)->first)
			) ? std::get<decltype(map_types.begin())>(its)->first : current_module
		), ...);

		assert(!current_module.empty());

		fn_cat(
			current_module,
			((
				std::get<decltype(map_types.begin())>(its) != map_types.end() &&
				current_module == std::get<decltype(map_types.begin())>(its)->first
			) ? std::get<decltype(map_types.begin())>(its)->second : decltype(map_types.begin()->second){})...
		);

		// increment participanting iterators
		((
			(
				std::get<decltype(map_types.begin())>(its) != map_types.end() &&
				current_module == std::get<decltype(map_types.begin())>(its)->first
			) ? void(++std::get<decltype(map_types.begin())>(its)) : void(0)
		), ...);
	}
}

//template<typename T>
//static void renderConfigCat(SimpleConfigModel& conf, std::string_view cmodule, std::string_view ccat, const std::pair<std::optional<T>, std::map<std::string, T>>& cat_dat) {
	//if (cat_dat.first.has_value()) {
		//T tmp = cat_dat.first.value();
		////if (ImGui::Checkbox(ccat.data(), &tmp)) {
			////conf.set(cmodule, ccat, tmp);
		////}
	//} else if (!cat_dat.second.empty()) {
		//ImGui::Text("uhhh");
	//}
//}
static void renderConfigCat(SimpleConfigModel& conf, std::string_view cmodule, std::string_view ccat, const std::pair<std::optional<bool>, std::map<std::string, bool>>& cat_dat) {
	if (cat_dat.first.has_value()) {
		bool tmp = cat_dat.first.value();
		if (ImGui::Checkbox(ccat.data(), &tmp)) {
			conf.set(cmodule, ccat, tmp);
		}
	} else if (!cat_dat.second.empty()) {
		ImGui::Text("uhhh");
	}
}
static void renderConfigCat(SimpleConfigModel& conf, std::string_view cmodule, std::string_view ccat, const std::pair<std::optional<int64_t>, std::map<std::string, int64_t>>& cat_dat) {
	if (cat_dat.first.has_value()) {
		int64_t tmp = cat_dat.first.value();
		if (ImGui::InputScalar(ccat.data(), ImGuiDataType_S64, &tmp)) {
			conf.set(cmodule, ccat, tmp);
		}
	} else if (!cat_dat.second.empty()) {
		ImGui::Text("uhhh");
	}
}
static void renderConfigCat(SimpleConfigModel& conf, std::string_view cmodule, std::string_view ccat, const std::pair<std::optional<double>, std::map<std::string, double>>& cat_dat) {
	if (cat_dat.first.has_value()) {
		double tmp = cat_dat.first.value();
		if (ImGui::InputDouble(ccat.data(), &tmp)) {
			conf.set(cmodule, ccat, tmp);
		}
	} else if (!cat_dat.second.empty()) {
		ImGui::Text("uhhh");
	}
}
static void renderConfigCat(SimpleConfigModel& conf, std::string_view cmodule, std::string_view ccat, const std::pair<std::optional<std::string>, std::map<std::string, std::string>>& cat_dat) {
	if (cat_dat.first.has_value()) {
		std::string tmp = cat_dat.first.value();
		if (ImGui::InputText(ccat.data(), &tmp)) {
			conf.set(cmodule, ccat, CM_ISV{tmp});
		}
	} else if (!cat_dat.second.empty()) {
		ImGui::Text("uhhh");
	}
}

SettingsWindow::SettingsWindow(SimpleConfigModel& conf) : _conf(conf) {
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
			ImGui::TextDisabled("most settings are only fetched once at program startup");

			// ufff
			// we need to iterate 4 maps, ideally in lexicographic order
			// (pain)
			// std::maps should be ordered more or less correctly
			// (less pain)
			// its recursive
			// (pain)
			// but the code is reusable
			// (less pain)

			iterateMultMaps(
				[this](std::string_view cmodule, const auto& ...mod_map_types) {
					if (ImGui::CollapsingHeader(cmodule.data())) {
						iterateMultMaps(
							[this, cmodule](std::string_view ccat, const auto& ...cat_map_types) {
								(renderConfigCat(_conf, cmodule, ccat, cat_map_types), ...);
							},
							mod_map_types...
						);
					}
				},
				_conf._map_bool,
				_conf._map_int,
				_conf._map_double,
				_conf._map_string
			);

#if 0
			auto it_b = _conf._map_bool.begin();
			auto it_i = _conf._map_int.begin();
			auto it_d = _conf._map_double.begin();
			auto it_s = _conf._map_string.begin();

			while (
				it_b != _conf._map_bool.end() ||
				it_i != _conf._map_int.end() ||
				it_d != _conf._map_double.end() ||
				it_s != _conf._map_string.end()
			) {
				// figure out current module
				std::string_view current_module;
				bool bool_is_curr = false;
				bool int_is_curr = false;
				bool double_is_curr = false;
				bool string_is_curr = false;

				if (it_b != _conf._map_bool.end() && (current_module.empty() || current_module >= it_b->first)) {
					//std::cerr << "SET bool\n";
					bool_is_curr = true;
					current_module = it_b->first;
				}
				if (it_i != _conf._map_int.end() && (current_module.empty() || current_module >= it_i->first)) {
					//std::cerr << "SET int\n";
					int_is_curr = true;
					current_module = it_i->first;
				}
				if (it_d != _conf._map_double.end() && (current_module.empty() || current_module >= it_d->first)) {
					//std::cerr << "SET double\n";
					double_is_curr = true;
					current_module = it_d->first;
				}
				if (it_s != _conf._map_string.end() && (current_module.empty() || current_module >= it_s->first)) {
					//std::cerr << "SET string\n";
					string_is_curr = true;
					current_module = it_s->first;
				}

				assert(!current_module.empty());

				//ImGui::Text("module '%s'", current_module.data());
				if (ImGui::CollapsingHeader(current_module.data())) {
					// ... do the whole 4way iterate again but on categoy
				}

				// after the module, increment iterators pointing to current_module
				if (bool_is_curr) {
					it_b++;
				}
				if (int_is_curr) {
					it_i++;
				}
				if (double_is_curr) {
					it_d++;
				}
				if (string_is_curr) {
					it_s++;
				}
			}
#endif
		}
		ImGui::End();
	}
}

