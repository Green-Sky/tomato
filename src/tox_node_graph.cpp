#include "./tox_node_graph.hpp"

#include <solanaceae/util/utils.hpp>

#include <imgui.h>

#include <cmath>


ToxNodeGraph::ToxNodeGraph(ToxEventProviderI& tep, ToxPrivateI* tpi) : _tpi(tpi), _tep_sr(tep.newSubRef(this)) {
	_tep_sr
		.subscribe(Tox_Event_Type::TOX_EVENT_DHT_NODES_RESPONSE)
	;
}

ToxNodeGraph::~ToxNodeGraph(void) {
}

float ToxNodeGraph::render(float time_delta) {
	if (ImGui::Begin("ToxNodeGraph")) {
		ImGui::Text("%zu nodes", _nodes.size());
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetTextLineHeight()*4);
		ImGui::InputScalar("steps", ImGuiDataType_U64, &_sim_steps);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetTextLineHeight()*10);
		ImGui::SliderFloat("squeeze", &_squeeze_hidden, 0.f, 2.f, "%.3f", ImGuiSliderFlags_Logarithmic);

		time_delta = std::min(time_delta, 0.1f);

		const float node_size = 1.f * ImGui::GetTextLineHeight() * 0.8f;
		const float push_apart_dist = 50.f * node_size;

		// spring sim
		for (size_t loop_n = 0; loop_n < _sim_steps; loop_n++) {
			for (auto& [o_k, o_v] : _nodes) {
				for (const auto& a_key : o_v.announced) {
					// pull torwards, stronger than push apart
					auto& a_n = _nodes.at(a_key);

					const float x_diff = a_n.x - o_v.x;
					const float y_diff = a_n.y - o_v.y;
					const float h0_diff = a_n.h0 - o_v.h0;
					const float dist = std::sqrt(x_diff*x_diff+y_diff*y_diff+h0_diff*h0_diff);
					const float strength = std::max(dist-node_size*2.f, 0.f) * 0.25f;

					// push self
					o_v.x += (x_diff/dist) * strength * time_delta;
					o_v.y += (y_diff/dist) * strength * time_delta;
					o_v.h0 += (h0_diff/dist) * strength * time_delta;

					// pull
					a_n.x -= (x_diff/dist) * strength * time_delta;
					a_n.y -= (y_diff/dist) * strength * time_delta;
					a_n.h0 -= (h0_diff/dist) * strength * time_delta;
				}

				// push away from everyone else
				for (const auto& [i_k, i_v] : _nodes) {
					if (i_k == o_k) {
						continue;
					}

					const float x_diff = i_v.x - o_v.x;
					const float y_diff = i_v.y - o_v.y;
					const float h0_diff = i_v.h0 - o_v.h0;
					const float dist_squared = x_diff*x_diff+y_diff*y_diff+h0_diff*h0_diff;

					if (dist_squared < node_size*node_size*4) {
						// account for h0?
						if (x_diff < 0.0001f && y_diff < 0.0001f) {
							// on same pos, jiggle
							o_v.x += ((_rng()%2000)/2000.f)-0.5f;
							o_v.y += ((_rng()%2000)/2000.f)-0.5f;
						}
					}
					if (dist_squared < push_apart_dist*push_apart_dist) {
						// push apart
						const float dist = std::sqrt(dist_squared);
						const float real_dist = std::max(dist - node_size*2, 0.f);
						// the min here is half the space between the centers, to allow some harder push apart forces
						const float strength = (node_size/(std::max(std::pow(real_dist, 1.2f), node_size))) * 6.f;

						// diff == o to i vec

						o_v.x -= (x_diff/dist) * strength * time_delta;
						o_v.y -= (y_diff/dist) * strength * time_delta;
						o_v.h0 -= (h0_diff/dist) * strength * time_delta;
					}
				}

				// squeeze hidden dimensions
				o_v.h0 -= o_v.h0 * _squeeze_hidden * time_delta;

				// clamp to pixel box?
				// 0,0 is center
				if (o_v.x < -_graph_size_x/2) {
					o_v.x = -_graph_size_x/2;
				} else if (o_v.x > _graph_size_x/2) {
					o_v.x = _graph_size_x/2;
				}
				if (o_v.y < -_graph_size_y/2) {
					o_v.y = -_graph_size_y/2;
				} else if (o_v.y > _graph_size_y/2) {
					o_v.y = _graph_size_y/2;
				}

				if (std::isnan(o_v.x) || std::isinf(o_v.x)) {
					o_v.x = 0.f;
				}
				if (std::isnan(o_v.y) || std::isinf(o_v.y)) {
					o_v.y = 0.f;
				}
				if (std::isnan(o_v.h0) || std::isinf(o_v.h0)) {
					o_v.h0 = 0.f;
				}
			}
		}

		const auto cursor_pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("hehe", {0.f, 0.f});
		{
			const auto [item_rect_x, item_rect_y] = ImGui::GetItemRectSize();
			_graph_size_x = item_rect_x;
			_graph_size_y = item_rect_y;
		}
		const bool area_hovered = ImGui::IsItemHovered();
		const auto mouse_pos = ImGui::GetMousePos();

		auto* dl = ImGui::GetWindowDrawList();
		for (const auto& node : _nodes) {
			for (const auto& target_key : node.second.announced) {
				const auto& target = _nodes[target_key];

				dl->AddLine(
					ImVec2(cursor_pos.x + (_graph_size_x/2) + node.second.x, cursor_pos.y + (_graph_size_y/2) + node.second.y),
					ImVec2(cursor_pos.x + (_graph_size_x/2) + target.x, cursor_pos.y + (_graph_size_y/2) + target.y),
					ImGui::GetColorU32({1.f, 1.f, 1.f, 0.5f})
				);
			}
		}

		auto hovered_node_it = _nodes.cend();
		for (auto it = _nodes.cbegin(); it != _nodes.cend(); it++) {
			const ImVec2 center {
				cursor_pos.x + (_graph_size_x/2) + it->second.x,
				cursor_pos.y + (_graph_size_y/2) + it->second.y,
			};
			dl->AddCircleFilled(
				center,
				node_size,
				ImGui::GetColorU32({1.f, 1.f, 1.f, 0.5f})
			);

			if (
				mouse_pos.x >= center.x - node_size && mouse_pos.x <= center.x + node_size &&
				mouse_pos.y >= center.y - node_size && mouse_pos.y <= center.y + node_size
			) {
				hovered_node_it = it;
			}
		}
		if (hovered_node_it != _nodes.cend() && ImGui::BeginTooltip()) {
			{
				const auto pubhex = bin2hex(ByteSpan{hovered_node_it->first.data.data(), hovered_node_it->first.data.size()});
				ImGui::TextUnformatted(pubhex.c_str());
			}
			ImGui::TextUnformatted("ips:");
			ImGui::Indent();
			for (const auto& ip : hovered_node_it->second.ips) {
				ImGui::TextUnformatted(ip.c_str());
			}
			ImGui::Unindent();
			ImGui::Separator();

			if (!hovered_node_it->second.announced.empty()) {
				ImGui::Text("announced:");
				for (const auto& it : hovered_node_it->second.announced) {
					const auto pubhex = bin2hex(ByteSpan{it.data.data(), it.data.size()});
					ImGui::TextUnformatted(pubhex.c_str());
				}
			}

			ImGui::EndTooltip();
		}

	}
	ImGui::End();

	return 100000.f;
}

bool ToxNodeGraph::onToxEvent(const Tox_Event_Dht_Nodes_Response* e) {
	const ToxKey src_pubkey{tox_event_dht_nodes_response_get_src_public_key(e), TOX_PUBLIC_KEY_SIZE};
	std::string_view src_ip{tox_event_dht_nodes_response_get_src_ip(e), tox_event_dht_nodes_response_get_src_ip_length(e)};

	const ToxKey node_pubkey{tox_event_dht_nodes_response_get_public_key(e), TOX_PUBLIC_KEY_SIZE};
	std::string_view node_ip{tox_event_dht_nodes_response_get_ip(e), tox_event_dht_nodes_response_get_ip_length(e)};

	const auto [src_it, src_new] = _nodes.emplace(src_pubkey, Node{});
	src_it->second.ips.emplace(src_ip);
	if (src_new) {
		src_it->second.x = ((_rng()%2000)/1000.f)-1.f;
		src_it->second.y = ((_rng()%2000)/1000.f)-1.f;
		src_it->second.h0 = ((_rng()%2000)/1000.f)-1.f;
	}

	const auto [node_it, node_new] = _nodes.emplace(node_pubkey, Node{});
	node_it->second.ips.emplace(node_ip);
	if (node_new) {
		// spawn new child at parent
		node_it->second.x = src_it->second.x;
		node_it->second.y = src_it->second.y;
		node_it->second.h0 = src_it->second.h0 + ((_rng()%2000)/1000.f)-1.f;
	}

	src_it->second.announced.emplace(node_pubkey);

	return false;
}
