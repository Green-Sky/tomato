#include "./stream_manager_ui.hpp"

#include <solanaceae/object_store/object_store.hpp>

#include <imgui/imgui.h>

StreamManagerUI::StreamManagerUI(ObjectStore2& os, StreamManager& sm) : _os(os), _sm(sm) {
}

void StreamManagerUI::render(void) {
	if (ImGui::Begin("StreamManagerUI")) {
		// TODO: node canvas?

		// by fametype ??

		ImGui::SeparatorText("Sources");

		// list sources
		for (const auto& [oc, ss] : _os.registry().view<Components::StreamSource>().each()) {
			ImGui::Text("src  %d (%s)[%s]", entt::to_integral(oc), ss.name.c_str(), ss.frame_type_name.c_str());
		}

		ImGui::SeparatorText("Sinks");

		// list sinks
		for (const auto& [oc, ss] : _os.registry().view<Components::StreamSink>().each()) {
			ImGui::Text("sink %d (%s)[%s]", entt::to_integral(oc), ss.name.c_str(), ss.frame_type_name.c_str());
		}

		ImGui::SeparatorText("Connections");

		// list connections
		for (const auto& con : _sm._connections) {
			ImGui::Text("con %d->%d", entt::to_integral(con->src.entity()), entt::to_integral(con->sink.entity()));
		}
	}
	ImGui::End();
}

