#include "./voip_chat_tab.hpp"

#include <solanaceae/contact/contact_store_i.hpp>
#include <solanaceae/object_store/object_store.hpp>

#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>

#include <imgui.h>

#include "../frame_streams/voip_model.hpp"

static void renderTab(ContactHandle4 c) {
	// guaranteed
	auto* voip_model = c.get<VoIPModelI*>();
	// use activesessioncomp instead?

	// HACK: direct access to os, remove this
	assert(c.registry()->ctx().contains<ObjectStore2>());
	auto& os = c.registry()->ctx().get<ObjectStore2>();

	// TODO: make this lookup trivial, so we can indicate running/ringing in tab title
	std::vector<ObjectHandle> contact_sessions;
	std::vector<ObjectHandle> acceptable_sessions;
	for (const auto& [ov, o_vm, sc] : os.registry().view<VoIPModelI*, Components::VoIP::SessionContact>().each()) {
		if (o_vm != voip_model) {
			continue;
		}
		if (sc.c != c) {
			continue;
		}

		auto o = os.objectHandle(ov);
		contact_sessions.push_back(o);

		if (!o.all_of<Components::VoIP::Incoming>()) {
			continue; // not incoming
		}

		// state is ringing/not yet accepted
		const auto* session_state = o.try_get<Components::VoIP::SessionState>();
		if (session_state == nullptr) {
			continue;
		}

		if (session_state->state != Components::VoIP::SessionState::State::RINGING) {
			continue;
		}
		acceptable_sessions.push_back(o);
	}

	if (!ImGui::BeginTabItem("VoIP", nullptr, (!contact_sessions.empty() ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None))) {
		return;
	}
	if (!ImGui::BeginChild("tab_content")) {
		ImGui::EndChild(); // always
		return;
	}

	// TODO: component? context?
	static Components::VoIP::DefaultConfig g_default_connections{
		true, true,
		true, false
	};

	if (ImGui::BeginTabBar("voip_tabbar")) {
		if (ImGui::BeginTabItem("Call")) {
			// TODO: it is possible that we want to support multi call per contact
			ImGui::BeginDisabled(!contact_sessions.empty());
			if (ImGui::Button("Start Call")) {
				voip_model->enter(c, g_default_connections);
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(acceptable_sessions.empty());
			if (ImGui::Button("Accept Call") && acceptable_sessions.size() < 2) {
				// optimized single session
				voip_model->accept(acceptable_sessions.front(), g_default_connections);
			} else if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_MouseButtonRight)) {
				for (const auto o : acceptable_sessions) {
					std::string label = "accept #";
					label += std::to_string(entt::to_integral(entt::to_entity(o.entity())));

					if (ImGui::MenuItem(label.c_str())) {
						voip_model->accept(o, g_default_connections);
					}
				}
				ImGui::EndPopup();
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(contact_sessions.empty());
			if (ImGui::Button("Leave/Reject Call") && contact_sessions.size() < 2) {
				// optimized single session
				voip_model->leave(contact_sessions.front());
			} else if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft | ImGuiPopupFlags_MouseButtonRight)) {
				for (const auto o : contact_sessions) {
					std::string label = "end #";
					label += std::to_string(entt::to_integral(entt::to_entity(o.entity())));

					if (ImGui::MenuItem(label.c_str())) {
						voip_model->leave(o);
					}
				}
				ImGui::EndPopup();
			}
			ImGui::EndDisabled();

			ImGui::Separator();

			ImGui::TextUnformatted("TODO: insert video here");

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Settings")) {
			ImGui::SeparatorText("Default Connections");
			ImGui::Checkbox("incoming audio", &g_default_connections.incoming_audio);
			ImGui::Checkbox("incoming video", &g_default_connections.incoming_video);
			ImGui::Separator();
			ImGui::Checkbox("outgoing audio", &g_default_connections.outgoing_audio);
			ImGui::Checkbox("outgoing video", &g_default_connections.outgoing_video);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::EndChild(); // always
	ImGui::EndTabItem();
}

bool registerVoIPChatTab(ContactStore4I& cs) {
	return cs.registerImGuiChatTab(
		entt::type_id<VoIPModelI*>().hash(), // not stable, but own it, so we dont care
		renderTab
	);
}

bool unregisterVoIPChatTab(ContactStore4I& cs) {
	return cs.unregisterImGuiChatTab(entt::type_id<VoIPModelI*>().hash());
}
