#include "./tox_dht_cap_histo.hpp"

#include <imgui/imgui.h>
#include <implot.h>

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
			if (_enabled && ImPlot::BeginPlot("##caphisto")) {
				ImPlot::SetupAxis(ImAxis_X1, "seconds", ImPlotAxisFlags_AutoFit);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Always);

				// TODO: fix colors

				ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.25f);
				ImPlot::PlotShaded("##ratio_shade", _ratios.data(), _ratios.size());
				ImPlot::PopStyleVar();

				ImPlot::PlotLine("##ratio", _ratios.data(), _ratios.size());

				ImPlot::EndPlot();
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

