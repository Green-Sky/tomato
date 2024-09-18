#include "./stream_manager_ui.hpp"

#include "./content/sdl_video_frame_stream2.hpp"
#include "./content/audio_stream.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <imgui/imgui.h>

#include <string>

StreamManagerUI::StreamManagerUI(ObjectStore2& os, StreamManager& sm) : _os(os), _sm(sm) {
}

void StreamManagerUI::render(void) {
	if (ImGui::Begin("StreamManagerUI")) {
		// TODO: node canvas?

		// by fametype ??

		ImGui::SeparatorText("Sources");

		// TODO: tables of id, button connect->to, name, type

		// list sources
		for (const auto& [oc, ss] : _os.registry().view<Components::StreamSource>().each()) {
			ImGui::Text("src  %d (%s)[%s]", entt::to_integral(entt::to_entity(oc)), ss.name.c_str(), ss.frame_type_name.c_str());
		}

		ImGui::SeparatorText("Sinks");

		// list sinks
		for (const auto& [oc, ss] : _os.registry().view<Components::StreamSink>().each()) {
			ImGui::PushID(entt::to_integral(oc));
			ImGui::Text("sink %d (%s)[%s]", entt::to_integral(entt::to_entity(oc)), ss.name.c_str(), ss.frame_type_name.c_str());

			if (ImGui::BeginPopupContextItem("sink_connect")) {
				if (ImGui::BeginMenu("connect video", ss.frame_type_name == entt::type_name<SDLVideoFrame>::value())) {
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
							_sm.connect<SDLVideoFrame>(oc_src, oc);
						}

						ImGui::PopID();
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("connect audio", ss.frame_type_name == entt::type_name<AudioFrame>::value())) {
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
							_sm.connect<AudioFrame>(oc_src, oc);
						}

						ImGui::PopID();
					}

					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		ImGui::SeparatorText("Connections");

		// TODO: table of id, button disconnect, context x->y, from name, to name, type?

		// list connections
		for (const auto& con : _sm._connections) {
			ImGui::Text("con %d->%d", entt::to_integral(entt::to_entity(con->src.entity())), entt::to_integral(entt::to_entity(con->sink.entity())));
		}
	}
	ImGui::End();
}

