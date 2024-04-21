#include "./contact_list.hpp"

#include <solanaceae/contact/components.hpp>

#include <imgui/imgui.h>
//#include <imgui/imgui_internal.h>

bool renderContactBig(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	const bool unread,
	const bool selectable,
	const bool selected
) {
	// we dont need ### bc there is no named prefix
	auto label = "##" + std::to_string(entt::to_integral(c.entity()));

	const bool request_incoming = c.all_of<Contact::Components::RequestIncoming>();
	const bool request_outgoing = c.all_of<Contact::Components::TagRequestOutgoing>();

	ImVec2 orig_curser_pos = ImGui::GetCursorPos();
	// HACK: fake selected to make it draw a box for us
	const bool show_selected = request_incoming || request_outgoing || selected;
	if (request_incoming) {
		ImGui::PushStyleColor(
			ImGuiCol_Header,
			th.getColor<ThemeCol_Contact::request_incoming>()
		);
	} else if (request_outgoing) {
		ImGui::PushStyleColor(
			ImGuiCol_Header,
			th.getColor<ThemeCol_Contact::request_outgoing>()
		);
	}

	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	bool got_selected = false;
	if (selectable) {
		got_selected = ImGui::Selectable(label.c_str(), show_selected, ImGuiSelectableFlags_None, {0, 3*TEXT_BASE_HEIGHT});
	} else {
		got_selected = ImGui::InvisibleButton(label.c_str(), {-FLT_MIN, 3*TEXT_BASE_HEIGHT});
	}

	if (request_incoming || request_outgoing) {
		ImGui::PopStyleColor();
	}

	if (ImGui::BeginItemTooltip()) {
		if (c.all_of<Contact::Components::ConnectionState>()) {
			const auto cstate = c.get<Contact::Components::ConnectionState>().state;
			ImGui::Text("Connection state: %s",
				(cstate == Contact::Components::ConnectionState::disconnected)
				? "offline"
				: (cstate == Contact::Components::ConnectionState::direct)
				? "online (direct)"
				: "online (cloud)"
			);
		} else {
			ImGui::TextUnformatted("Connection state: unknown");
		}

		ImGui::EndTooltip();
	}

	ImVec2 post_curser_pos = ImGui::GetCursorPos();

	ImVec2 img_curser {
		orig_curser_pos.x + ImGui::GetStyle().FramePadding.x,
		orig_curser_pos.y + ImGui::GetStyle().FramePadding.y
	};

	float img_y {
		//(post_curser_pos.y - orig_curser_pos.y) - ImGui::GetStyle().FramePadding.y*2
		TEXT_BASE_HEIGHT*3 - ImGui::GetStyle().FramePadding.y*2
	};

	ImGui::SetCursorPos(img_curser);

	// TODO: refactor out avatar (with online state overlay)

	ImVec4 color_current = th.getColor<ThemeCol_Contact::avatar_offline>();
	if (c.all_of<Contact::Components::ConnectionState>()) {
		const auto c_state = c.get<Contact::Components::ConnectionState>().state;
		if (c_state == Contact::Components::ConnectionState::State::direct) {
			color_current = th.getColor<ThemeCol_Contact::avatar_online_direct>();
		} else if (c_state == Contact::Components::ConnectionState::State::cloud) {
			color_current = th.getColor<ThemeCol_Contact::avatar_online_cloud>();
		}
	}

	// avatar
	const auto [id, width, height] = contact_tc.get(c);
	ImGui::Image(
		id,
		ImVec2{img_y, img_y},
		{0, 0},
		{1, 1},
		{1, 1, 1, 1},
		color_current
	);

	ImGui::SameLine();
	ImGui::BeginGroup();
	{
		// TODO: is there a better way?
		// maybe cache mm?
		//bool has_unread = false;
#if 0
		if (const auto* mm = _rmm.get(c); mm != nullptr) {
			if (const auto* unread_storage = mm->storage<Message::Components::TagUnread>(); unread_storage != nullptr && !unread_storage->empty()) {
#if 0
				assert(unread_storage.size() == 0);
				assert(unread_storage.cbegin() == unread_storage.cend());
				std::cout << "UNREAD ";
				for (const auto e : mm->view<Message::Components::TagUnread>()) {
					std::cout << entt::to_integral(e) << " ";
				}
				std::cout << "\n";
#endif
				has_unread = true;
			}
		}
#endif

		ImGui::Text("%s%s", unread?"* ":"", (c.all_of<Contact::Components::Name>() ? c.get<Contact::Components::Name>().name.c_str() : "<unk>"));

		if (request_incoming) {
			ImGui::TextUnformatted("Incoming request/invite");
		} else if (request_outgoing) {
			ImGui::TextUnformatted("Outgoing request/invite");
		} else {
			//ImGui::Text("status message...");
		}
		//constexpr std::string_view test_text{"text"};
		//ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), ImVec2{}, ImVec2{}, 1.f, 1.f, test_text.data(), test_text.data()+test_text.size(), nullptr);
	}
	ImGui::EndGroup();

	ImGui::SetCursorPos(post_curser_pos);

	return got_selected;
}

