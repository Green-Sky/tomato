#include "./tox_dht_cap_histo.hpp"

#include <imgui/imgui.h>

void ToxDHTCapHisto::tick(float time_delta) {
	if (!_enabled) {
		return;
	}

	_time_since_last_add += time_delta;
	if (_time_since_last_add >= _value_add_interval) {
		_time_since_last_add = 0.f; // very loose

		const auto total = _tpi.toxDHTGetNumCloselist();
		const auto with_cap = _tpi.toxDHTGetNumCloselistAnnounceCapable();

		if (total == 0 || with_cap == 0) {
			_ratios.push_back(0.f);
		} else {
			_ratios.push_back(float(with_cap) / float(total));
		}

		// TODO: limit
		while (_ratios.size() > 5*60) {
			_ratios.erase(_ratios.begin());
		}
	}
}

void ToxDHTCapHisto::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Separator();
				if (ImGui::BeginMenu("Tox")) {
					ImGui::SeparatorText("DHT diagnostics");

					ImGui::Checkbox("enabled", &_enabled);

					if (ImGui::MenuItem("announce capability histogram", nullptr, _show_window)) {
						_show_window = !_show_window;
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}


	if (_show_window) {
		if (ImGui::Begin("Tox DHT announce capability histogram", &_show_window)) {
			if (_enabled) {
				ImGui::PlotHistogram("##histogram", _ratios.data(), _ratios.size(), 0, nullptr, 0.f, 1.f, {-1, -1});
			} else {
				ImGui::TextUnformatted("logging disabled!");
				if (ImGui::Button("enable")) {
					_enabled = true;
				}
			}
		}
		ImGui::End();
	}
}

