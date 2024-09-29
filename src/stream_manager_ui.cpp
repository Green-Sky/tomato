#include "./stream_manager_ui.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <imgui/imgui.h>

#include <string>

StreamManagerUI::StreamManagerUI(ObjectStore2& os, StreamManager& sm) : _os(os), _sm(sm) {
}

void StreamManagerUI::render(void) {
	{ // main window menubar injection
		// assumes the window "tomato" was rendered already by cg
		if (ImGui::Begin("tomato")) {
			if (ImGui::BeginMenuBar()) {
				// TODO: drop all menu sep?
				//ImGui::Separator(); // os already exists (very hacky)
				if (ImGui::BeginMenu("ObjectStore")) {
					if (ImGui::MenuItem("Stream Manger", nullptr, _show_window)) {
						_show_window = !_show_window;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

		}
		ImGui::End();
	}


	if (!_show_window) {
		return;
	}

	if (ImGui::Begin("StreamManagerUI", &_show_window)) {
		// TODO: node canvas

		// by fametype ??

		if (ImGui::CollapsingHeader("Sources", ImGuiTreeNodeFlags_DefaultOpen)) {
			// list sources
			if (ImGui::BeginTable("sources_and_sinks", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("id");
				ImGui::TableSetupColumn("name");
				ImGui::TableSetupColumn("##conn");
				ImGui::TableSetupColumn("type");

				ImGui::TableHeadersRow();

				for (const auto& [oc, ss] : _os.registry().view<Components::StreamSource>().each()) {
					//ImGui::Text("src  %d (%s)[%s]", entt::to_integral(entt::to_entity(oc)), ss.name.c_str(), ss.frame_type_name.c_str());
					ImGui::PushID(entt::to_integral(oc));

					ImGui::TableNextColumn();
					ImGui::Text("%d", entt::to_integral(entt::to_entity(oc)));

					if (_os.registry().all_of<Components::TagDefaultTarget>(oc)) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.6f, 0.f, 0.6f, 0.25f}));
					} else if (_os.registry().all_of<Components::TagConnectToDefault>(oc)) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.6f, 0.6f, 0.f, 0.25f}));
					}

					const auto *ssrc = _os.registry().try_get<Components::StreamSource>(oc);
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssrc!=nullptr?ssrc->name.c_str():"none");

					ImGui::TableNextColumn();
					if (ImGui::SmallButton("->")) {
						ImGui::OpenPopup("src_connect");
					}
					if (ImGui::BeginPopup("src_connect")) {
						if (ImGui::BeginMenu("connect to")) {
							for (const auto& [oc_sink, s_sink] : _os.registry().view<Components::StreamSink>().each()) {
								if (s_sink.frame_type_name != ss.frame_type_name) {
									continue;
								}

								ImGui::PushID(entt::to_integral(oc_sink));

								std::string sink_label {"src "};
								sink_label += std::to_string(entt::to_integral(entt::to_entity(oc_sink)));
								sink_label += " (";
								sink_label += s_sink.name;
								sink_label += ")[";
								sink_label += s_sink.frame_type_name;
								sink_label += "]";
								if (ImGui::MenuItem(sink_label.c_str())) {
									_sm.connect(oc, oc_sink);
								}

								ImGui::PopID();
							}

							ImGui::EndMenu();
						}
						ImGui::EndPopup();
					}

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssrc!=nullptr?ssrc->frame_type_name.c_str():"???");

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		} // sources header

		if (ImGui::CollapsingHeader("Sinks", ImGuiTreeNodeFlags_DefaultOpen)) {
			// list sinks
			if (ImGui::BeginTable("sources_and_sinks", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("id");
				ImGui::TableSetupColumn("name");
				ImGui::TableSetupColumn("##conn");
				ImGui::TableSetupColumn("type");

				ImGui::TableHeadersRow();

				for (const auto& [oc, ss] : _os.registry().view<Components::StreamSink>().each()) {
					ImGui::PushID(entt::to_integral(oc));

					ImGui::TableNextColumn();
					ImGui::Text("%d", entt::to_integral(entt::to_entity(oc)));

					if (_os.registry().all_of<Components::TagDefaultTarget>(oc)) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.6f, 0.f, 0.6f, 0.25f}));
					} else if (_os.registry().all_of<Components::TagConnectToDefault>(oc)) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImVec4{0.6f, 0.6f, 0.f, 0.25f}));
					}

					const auto *ssink = _os.registry().try_get<Components::StreamSink>(oc);
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssink!=nullptr?ssink->name.c_str():"none");

					ImGui::TableNextColumn();
					if (ImGui::SmallButton("->")) {
						ImGui::OpenPopup("sink_connect");
					}
					// ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings
					if (ImGui::BeginPopup("sink_connect")) {
						if (ImGui::BeginMenu("connect to")) {
							for (const auto& [oc_src, s_src] : _os.registry().view<Components::StreamSource>().each()) {
								if (s_src.frame_type_name != ss.frame_type_name) {
									continue;
								}

								ImGui::PushID(entt::to_integral(oc_src));

								std::string source_label {"src "};
								source_label += std::to_string(entt::to_integral(entt::to_entity(oc_src)));
								source_label += " (";
								source_label += s_src.name;
								source_label += ")[";
								source_label += s_src.frame_type_name;
								source_label += "]";
								if (ImGui::MenuItem(source_label.c_str())) {
									_sm.connect(oc_src, oc);
								}

								ImGui::PopID();
							}

							ImGui::EndMenu();
						}
						ImGui::EndPopup();
					}

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssink!=nullptr?ssink->frame_type_name.c_str():"???");

					ImGui::PopID();
				}

				ImGui::EndTable();
			}
		} // sink header

		if (ImGui::CollapsingHeader("Connections", ImGuiTreeNodeFlags_DefaultOpen)) {
			// list connections
			if (ImGui::BeginTable("connections", 6, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
				ImGui::TableSetupColumn("##id"); // TODO: remove?
				ImGui::TableSetupColumn("##disco");
				ImGui::TableSetupColumn("##qdesc");
				ImGui::TableSetupColumn("from");
				ImGui::TableSetupColumn("to");
				ImGui::TableSetupColumn("type");

				ImGui::TableHeadersRow();

				for (size_t i = 0; i < _sm._connections.size(); i++) {
					const auto& con = _sm._connections[i];
					//ImGui::Text("con %d->%d", entt::to_integral(entt::to_entity(con->src.entity())), entt::to_integral(entt::to_entity(con->sink.entity())));

					ImGui::PushID(i);

					ImGui::TableNextColumn();
					ImGui::Text("%zu", i); // do connections have ids?

					ImGui::TableNextColumn();
					if (ImGui::SmallButton("X")) {
						con->stop = true;
					}

					ImGui::TableNextColumn();
					ImGui::Text("%d->%d", entt::to_integral(entt::to_entity(con->src.entity())), entt::to_integral(entt::to_entity(con->sink.entity())));

					const auto *ssrc = con->src.try_get<Components::StreamSource>();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssrc!=nullptr?ssrc->name.c_str():"none");

					const auto *ssink = con->sink.try_get<Components::StreamSink>();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted(ssink!=nullptr?ssink->name.c_str():"none");

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(
						(ssrc!=nullptr)?
							ssrc->frame_type_name.c_str():
							(ssink!=nullptr)?
								ssink->frame_type_name.c_str()
								:"???"
					);

					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		} // con header
	}
	ImGui::End();
}

