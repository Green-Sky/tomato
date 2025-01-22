#include "./tox_netprof_ui.hpp"

#include <imgui/imgui.h>
#include <implot.h>

#include <string>
#include <cmath>

#include "./string_formatter_utils.hpp"

static const char* typedPkgIDToString(Tox_Netprof_Packet_Type type, uint8_t id) {
	// pain
	if (type == TOX_NETPROF_PACKET_TYPE_UDP) {
		switch (id) {
			case TOX_NETPROF_PACKET_ID_ZERO: return "Ping request";
			case TOX_NETPROF_PACKET_ID_ONE: return "Ping response";
			case TOX_NETPROF_PACKET_ID_TWO: return "Get nodes request";
			case TOX_NETPROF_PACKET_ID_FOUR: return "Send nodes response";
			case TOX_NETPROF_PACKET_ID_COOKIE_REQUEST: return "Cookie request";
			case TOX_NETPROF_PACKET_ID_COOKIE_RESPONSE: return "Cookie response";
			case TOX_NETPROF_PACKET_ID_CRYPTO_HS: return "Crypto handshake";
			case TOX_NETPROF_PACKET_ID_CRYPTO_DATA: return "Crypto data";
			case TOX_NETPROF_PACKET_ID_CRYPTO: return "Encrypted data";
			case TOX_NETPROF_PACKET_ID_LAN_DISCOVERY: return "LAN discovery";
			case TOX_NETPROF_PACKET_ID_GC_HANDSHAKE: return "DHT groupchat handshake";
			case TOX_NETPROF_PACKET_ID_GC_LOSSLESS: return "DHT groupchat lossless";
			case TOX_NETPROF_PACKET_ID_GC_LOSSY: return "DHT groupchat lossy";
			case TOX_NETPROF_PACKET_ID_ONION_SEND_INITIAL: return "Onion send init";
			case TOX_NETPROF_PACKET_ID_ONION_SEND_1: return "Onion send 1";
			case TOX_NETPROF_PACKET_ID_ONION_SEND_2: return "Onion send 2";
			case TOX_NETPROF_PACKET_ID_ANNOUNCE_REQUEST_OLD: return "DHT announce request (old)";
			case TOX_NETPROF_PACKET_ID_ANNOUNCE_RESPONSE_OLD: return "DHT announce response (old)";
			case TOX_NETPROF_PACKET_ID_ONION_DATA_REQUEST: return "Onion data request";
			case TOX_NETPROF_PACKET_ID_ONION_DATA_RESPONSE: return "Onion data response";
			case TOX_NETPROF_PACKET_ID_ANNOUNCE_REQUEST: return "DHT announce request";
			case TOX_NETPROF_PACKET_ID_ANNOUNCE_RESPONSE: return "DHT announce response";
			case TOX_NETPROF_PACKET_ID_ONION_RECV_3: return "Onion receive 3";
			case TOX_NETPROF_PACKET_ID_ONION_RECV_2: return "Onion receive 2";
			case TOX_NETPROF_PACKET_ID_ONION_RECV_1: return "Onion receive 1";
			case TOX_NETPROF_PACKET_ID_FORWARD_REQUEST: return "DHT forwarding request";
			case TOX_NETPROF_PACKET_ID_FORWARDING: return "DHT forwarding";
			case TOX_NETPROF_PACKET_ID_FORWARD_REPLY: return "DHT forward reply";
			case TOX_NETPROF_PACKET_ID_DATA_SEARCH_REQUEST: return "DHT data search request";
			case TOX_NETPROF_PACKET_ID_DATA_SEARCH_RESPONSE: return "DHT data search response";
			case TOX_NETPROF_PACKET_ID_DATA_RETRIEVE_REQUEST: return "DHT data retrieve request";
			case TOX_NETPROF_PACKET_ID_DATA_RETRIEVE_RESPONSE: return "DHT data retrieve response";
			case TOX_NETPROF_PACKET_ID_STORE_ANNOUNCE_REQUEST: return "DHT store announce request";
			case TOX_NETPROF_PACKET_ID_STORE_ANNOUNCE_RESPONSE: return "DHT store announce response";
			case TOX_NETPROF_PACKET_ID_BOOTSTRAP_INFO: return "Bootstrap info";
		}
	} else if (type == TOX_NETPROF_PACKET_TYPE_TCP) { // TODO: or client/server
		switch (id) {
			case TOX_NETPROF_PACKET_ID_ZERO: return "Routing request";
			case TOX_NETPROF_PACKET_ID_ONE: return "Routing response";
			case TOX_NETPROF_PACKET_ID_TWO: return "Connection notification";
			case TOX_NETPROF_PACKET_ID_TCP_DISCONNECT: return "disconnect notification";
			case TOX_NETPROF_PACKET_ID_FOUR: return "Ping packet";
			case TOX_NETPROF_PACKET_ID_TCP_PONG: return "pong packet";
			case TOX_NETPROF_PACKET_ID_TCP_OOB_SEND: return "out-of-band send";
			case TOX_NETPROF_PACKET_ID_TCP_OOB_RECV: return "out-of-band receive";
			case TOX_NETPROF_PACKET_ID_TCP_ONION_REQUEST: return "onion request";
			case TOX_NETPROF_PACKET_ID_TCP_ONION_RESPONSE: return "onion response";
			case TOX_NETPROF_PACKET_ID_TCP_DATA: return "data";
			//case TOX_NETPROF_PACKET_ID_BOOTSTRAP_INFO: return "Bootstrap info";
		}
	}

	return "UNK";
}

void ToxNetprofUI::tick(float time_delta) {
	if (!_enabled) {
		return;
	}

	_time_since_last_add += time_delta;
	if (_time_since_last_add >= _value_add_interval) {
		if (_time_since_last_add >= 20.f * _value_add_interval) {
			_time_since_last_add = 0.f; // cut our losses
		} else {
			_time_since_last_add -= _value_add_interval;
		}

		if (_udp_tbtx.empty()) {
			_udp_tbtx.push_back(0.f);
			_udp_tbtx_prev = _tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_SENT);
		} else {
			const auto new_value = _tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_SENT);
			_udp_tbtx.push_back(new_value - _udp_tbtx_prev);
			_udp_tbtx_prev = new_value;
		}

		if (_udp_tbrx.empty()) {
			_udp_tbrx.push_back(0.f);
			_udp_tbrx_prev = _tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_RECV);
		} else {
			const auto new_value = _tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_RECV);
			_udp_tbrx.push_back(new_value - _udp_tbrx_prev);
			_udp_tbrx_prev = new_value;
		}

		// TODO: limit
		while (_udp_tbtx.size() > 5*60) {
			_udp_tbtx.erase(_udp_tbtx.begin());
		}

		while (_udp_tbrx.size() > 5*60) {
			_udp_tbrx.erase(_udp_tbrx.begin());
		}

		_udp_tbtx_avg = 0.f;
		if (!_udp_tbtx.empty()) {
			for (const auto bytes : _udp_tbtx) {
				_udp_tbtx_avg += bytes;
			}
			_udp_tbtx_avg /= (_udp_tbtx.size() * _value_add_interval);
		}

		_udp_tbrx_avg = 0.f;
		if (!_udp_tbrx.empty()) {
			for (const auto bytes : _udp_tbrx) {
				_udp_tbrx_avg += bytes;
			}
			_udp_tbrx_avg /= (_udp_tbrx.size() * _value_add_interval);
		}
	}
}

float ToxNetprofUI::render(float time_delta) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("Tox")) {
					ImGui::SeparatorText("Net diagnostics");

					if (ImGui::MenuItem("Breakdown table", nullptr, _show_window_table)) {
						_show_window_table = !_show_window_table;
					}

					ImGui::Checkbox("histogram logging", &_enabled);

					if (ImGui::MenuItem("Netprof histograms", nullptr, _show_window_histo)) {
						_show_window_histo = !_show_window_histo;
					}

					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
		}
		ImGui::End();
	}

	if (_show_window_table) {
		if (ImGui::Begin("Tox Netprof table", &_show_window_table)) {
			ImGui::Text("UDP total Count tx/rx: %zu/%zu",
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_RECV)
			);
			ImGui::Text("UDP total Bytes tx/rx: %zu/%zu",
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_UDP, TOX_NETPROF_DIRECTION_RECV)
			);
			ImGui::Text("TCP total Count tx/rx: %zu/%zu (client: %zu/%zu; server: %zu/%zu)",
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP, TOX_NETPROF_DIRECTION_RECV),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP_CLIENT, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP_CLIENT, TOX_NETPROF_DIRECTION_RECV),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP_SERVER, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalCount(TOX_NETPROF_PACKET_TYPE_TCP_SERVER, TOX_NETPROF_DIRECTION_RECV)
			);
			ImGui::Text("TCP total Bytes tx/rx: %zu/%zu (client: %zu/%zu; server: %zu/%zu)",
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP, TOX_NETPROF_DIRECTION_RECV),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP_CLIENT, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP_CLIENT, TOX_NETPROF_DIRECTION_RECV),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP_SERVER, TOX_NETPROF_DIRECTION_SENT),
				_tpi.toxNetprofGetPacketTotalBytes(TOX_NETPROF_PACKET_TYPE_TCP_SERVER, TOX_NETPROF_DIRECTION_RECV)
			);

			// TODO: color types (tcp/udp and maybe dht)

			static float decay_rate = 3.f;
			ImGui::SliderFloat("heat decay (/s)", &decay_rate, 0.f, 50.0f);

			// type (udp/tcp), id/name, avg size, count tx, count rx, bytes tx, bytes rx
			if (ImGui::BeginTable("per packet", 7, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {

				ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
				ImGui::TableSetupColumn("type");
				ImGui::TableSetupColumn("pkt type");
				ImGui::TableSetupColumn("avg size");
				ImGui::TableSetupColumn("count tx");
				ImGui::TableSetupColumn("count rx");
				ImGui::TableSetupColumn("bytes tx");
				ImGui::TableSetupColumn("bytes rx");

				ImGui::TableHeadersRow();

				auto value_fn = [time_delta](size_t i, uint64_t value, auto& prev_map, auto& heat_map, const float scale = 0.2f) {
					ImGui::TableNextColumn();
					ImGui::Text("%zu", value);
					if (prev_map.count(i)) {
						const auto delta = value - prev_map[i];
						float& heat = heat_map[i];
						heat += delta * scale; // count 0.1, bytes 0.02?

						// TODO: actual color function
						float green = 0.7f;
						if (heat > 1.f) {
							green -= (heat - 1.f) * 0.1f;
						}

						ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.9f, green, 0.0f, heat)));

						//ImGui::SameLine();
						//ImGui::Text("%.2f", heat);

						//heat *= 0.94f;
						float decay = decay_rate * time_delta;
						if (decay > 0.999f) {
							decay = 0.999f;
						}
						heat *= (1.f - decay);
					}
					prev_map[i] = value;
				};

				for (size_t i = 0; i < 0xff; i++) {
					// FIXME: netprof sums up a range on 0x10, even for udp
					if (i == 0x10) {
						continue;
					}
					const auto count_sent = _tpi.toxNetprofGetPacketIdCount(TOX_NETPROF_PACKET_TYPE_UDP, i, TOX_NETPROF_DIRECTION_SENT);
					const auto count_received = _tpi.toxNetprofGetPacketIdCount(TOX_NETPROF_PACKET_TYPE_UDP, i, TOX_NETPROF_DIRECTION_RECV);

					if (count_sent == 0 && count_received == 0) {
						continue; // skip empty
					}

					const auto bytes_sent = _tpi.toxNetprofGetPacketIdBytes(TOX_NETPROF_PACKET_TYPE_UDP, i, TOX_NETPROF_DIRECTION_SENT);
					const auto bytes_received = _tpi.toxNetprofGetPacketIdBytes(TOX_NETPROF_PACKET_TYPE_UDP, i, TOX_NETPROF_DIRECTION_RECV);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted("UDP");

					ImGui::TableNextColumn();
					ImGui::Text("%02zx(%s)", i, typedPkgIDToString(TOX_NETPROF_PACKET_TYPE_UDP, i));

					ImGui::TableNextColumn();
					ImGui::Text("%g", double(bytes_sent+bytes_received) / double(count_sent+count_received));

					value_fn(i, count_sent, _udp_ctx_prev, _udp_ctx_heat);

					value_fn(i, count_received, _udp_crx_prev, _udp_crx_heat);

					value_fn(i, bytes_sent, _udp_btx_prev, _udp_btx_heat, 0.005f);

					value_fn(i, bytes_received, _udp_brx_prev, _udp_brx_heat, 0.005f);
				}

				for (size_t i = 0; i <= 0x10; i++) {
					const auto count_sent = _tpi.toxNetprofGetPacketIdCount(TOX_NETPROF_PACKET_TYPE_TCP, i, TOX_NETPROF_DIRECTION_SENT);
					const auto count_received = _tpi.toxNetprofGetPacketIdCount(TOX_NETPROF_PACKET_TYPE_TCP, i, TOX_NETPROF_DIRECTION_RECV);

					if (count_sent == 0 && count_received == 0) {
						continue; // skip empty
					}

					const auto bytes_sent = _tpi.toxNetprofGetPacketIdBytes(TOX_NETPROF_PACKET_TYPE_TCP, i, TOX_NETPROF_DIRECTION_SENT);
					const auto bytes_received = _tpi.toxNetprofGetPacketIdBytes(TOX_NETPROF_PACKET_TYPE_TCP, i, TOX_NETPROF_DIRECTION_RECV);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted("TCP");

					ImGui::TableNextColumn();
					ImGui::Text("%02zx(%s)", i, typedPkgIDToString(TOX_NETPROF_PACKET_TYPE_TCP, i));

					ImGui::TableNextColumn();
					ImGui::Text("%g", double(bytes_sent+bytes_received) / double(count_sent+count_received));

					value_fn(i, count_sent, _tcp_ctx_prev, _tcp_ctx_heat);

					value_fn(i, count_received, _tcp_crx_prev, _tcp_crx_heat);

					value_fn(i, bytes_sent, _tcp_btx_prev, _tcp_btx_heat, 0.005f);

					value_fn(i, bytes_received, _tcp_brx_prev, _tcp_brx_heat, 0.005f);
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}

	if (_show_window_histo) {
		if (ImGui::Begin("Tox Netprof graph", &_show_window_histo)) {
			if (_enabled && ImPlot::BeginPlot("##plot")) {
				ImPlot::SetupAxes(nullptr, "bytes/second", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

				ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, 0.f, INFINITY);

				// TODO: bytes tick interval of 1024 (or smaller like multiples of 2)
				ImPlot::SetupAxisFormat(
					ImAxis_Y1,
					// TODO: extract
					+[](double value, char* buff, int size, void*) -> int {
						if (value < 0) {
							return 0;
						}

						const char* byte_suffix = "???";
						int64_t byte_divider = sizeToHumanReadable(value, byte_suffix);

						return snprintf(buff, size, "%g %s/s", value/byte_divider, byte_suffix);
					},
					nullptr
				);

				{
					ImPlot::PlotLine(
						"udp tx/s",
						_udp_tbtx.data(), _udp_tbtx.size(),
						_value_add_interval // normalize to /s
					);
					auto item_color = ImPlot::GetLastItemColor();
					item_color.w *= 0.5f;
					ImPlot::SetNextLineStyle(item_color);
					ImPlot::PlotInfLines("udp tx avg", &_udp_tbtx_avg, 1, ImPlotInfLinesFlags_Horizontal);
				}

				{
					ImPlot::PlotLine(
						"udp rx/s",
						_udp_tbrx.data(), _udp_tbrx.size(),
						_value_add_interval // normalize to /s
					);
					auto item_color = ImPlot::GetLastItemColor();
					item_color.w *= 0.5f;
					ImPlot::SetNextLineStyle(item_color);
					ImPlot::PlotInfLines("udp rx avg", &_udp_tbrx_avg, 1, ImPlotInfLinesFlags_Horizontal);
				}

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

	if (_show_window_table) {
		return 0.1f; // min 10fps
	}
	return 2.f;
}

