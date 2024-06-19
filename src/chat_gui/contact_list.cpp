#include "./contact_list.hpp"

#include <solanaceae/contact/components.hpp>

#include <imgui/imgui.h>
//#include <imgui/imgui_internal.h>

#include "./icons/direct.hpp"
#include "./icons/cloud.hpp"
#include "./icons/mail.hpp"

void renderAvatar(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	ImVec2 box
) {
	// deploy dummy of same size and check visibility
	const auto orig_curser_pos = ImGui::GetCursorPos();
	ImGui::Dummy(box);
	if (ImGui::IsItemVisible()) {
		ImGui::SetCursorPos(orig_curser_pos); // reset for actual img

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
			box,
			{0, 0},
			{1, 1},
			{1, 1, 1, 1},
			color_current
		);
	}
}

bool renderContactBig(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	int line_height,
	const bool unread,
	const bool selectable,
	const bool selected
) {
	ImGui::BeginGroup();
	if (line_height < 1) {
		line_height = 1;
	}

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
		got_selected = ImGui::Selectable(label.c_str(), show_selected, ImGuiSelectableFlags_None, {0, line_height*TEXT_BASE_HEIGHT});
	} else {
		got_selected = ImGui::InvisibleButton(label.c_str(), {-FLT_MIN, line_height*TEXT_BASE_HEIGHT});
	}

	if (request_incoming || request_outgoing) {
		ImGui::PopStyleColor();
	}

	const auto* cstate = c.try_get<Contact::Components::ConnectionState>();
	if (ImGui::BeginItemTooltip()) {
		if (cstate != nullptr) {
			ImGui::Text("Connection state: %s",
				(cstate->state == Contact::Components::ConnectionState::disconnected)
				? "offline"
				: (cstate->state == Contact::Components::ConnectionState::direct)
				? "online (direct)"
				: "online (cloud)"
			);
		} else {
			ImGui::TextUnformatted("Connection state: unknown");
		}

		if (
			const auto* slt = c.try_get<Contact::Components::StatusText>();
			slt != nullptr &&
			!slt->text.empty()
		) {
			ImGui::SeparatorText("Status Text");
			//ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
			ImGui::TextUnformatted(slt->text.c_str());
			//ImGui::PopStyleColor();
		}

		// TODO: add a whole bunch more info
		ImGui::EndTooltip();
	}

	ImVec2 post_curser_pos = ImGui::GetCursorPos();

	ImVec2 img_curser {
		orig_curser_pos.x + ImGui::GetStyle().FramePadding.x,
		orig_curser_pos.y + ImGui::GetStyle().FramePadding.y
	};

	float img_y {TEXT_BASE_HEIGHT*line_height - ImGui::GetStyle().FramePadding.y*2};

	ImGui::SetCursorPos(img_curser);

	renderAvatar(th, contact_tc, c, {img_y, img_y});

	const float same_line_spacing = ImGui::GetStyle().ItemSpacing.x*0.5f;
	ImGui::SameLine(0.f, same_line_spacing);
	ImGui::BeginGroup();
	{
		{ // line 1
			if (line_height == 1 && cstate != nullptr) {
				// icon pos
				auto p0 = ImGui::GetCursorScreenPos();
				p0.y += ImGui::GetStyle().FramePadding.y;
				ImVec2 p1_o = {img_y, img_y}; // img_y is 1 line_height in this case

				const ImU32 col_back = ImGui::GetColorU32(th.getColor<ThemeCol_Contact::icon_backdrop>());
				if (cstate->state == Contact::Components::ConnectionState::direct) { // direct icon
					drawIconDirect(
						p0,
						p1_o,
						ImGui::GetColorU32(th.getColor<ThemeCol_Contact::avatar_online_direct>()),
						col_back
					);
				} else if (cstate->state == Contact::Components::ConnectionState::cloud) { // cloud icon
					drawIconCloud(
						p0,
						p1_o,
						ImGui::GetColorU32(th.getColor<ThemeCol_Contact::avatar_online_cloud>()),
						col_back
					);
				}
				ImGui::Dummy(p1_o);
				ImGui::SameLine(0.f, same_line_spacing);
			}

			//ImGui::Text("%s%s", unread?"* ":"", (c.all_of<Contact::Components::Name>() ? c.get<Contact::Components::Name>().name.c_str() : "<unk>"));
			ImGui::TextUnformatted(c.all_of<Contact::Components::Name>() ? c.get<Contact::Components::Name>().name.c_str() : "<unk>");
			if (unread) {
				ImGui::SameLine();
				const float icon_size { TEXT_BASE_HEIGHT - ImGui::GetStyle().FramePadding.y*2 };
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - icon_size);

				// icon pos
				auto p0 = ImGui::GetCursorScreenPos();
				//p0.y += ImGui::GetStyle().FramePadding.y;
				ImVec2 p1_o = {icon_size, icon_size};

				drawIconMail(
					p0,
					p1_o,
					ImGui::GetColorU32(th.getColor<ThemeCol_Contact::unread>()),
					ImGui::GetColorU32(th.getColor<ThemeCol_Contact::icon_backdrop>())
				);
				ImGui::Dummy(p1_o);
			}
		}

		// line 2
		if (line_height >= 2) {
			if (request_incoming) {
				ImGui::TextUnformatted("Incoming request/invite");
			} else if (request_outgoing) {
				ImGui::TextUnformatted("Outgoing request/invite");
			} else {
				if (cstate != nullptr) {
					// icon pos
					auto p0 = ImGui::GetCursorScreenPos();
					p0.y += ImGui::GetStyle().FramePadding.y;
					const float box_hight = TEXT_BASE_HEIGHT - ImGui::GetStyle().FramePadding.y*2;
					ImVec2 p1_o = {box_hight, box_hight};

					const ImU32 col_back = ImGui::GetColorU32(th.getColor<ThemeCol_Contact::icon_backdrop>());
					if (cstate->state == Contact::Components::ConnectionState::direct) { // direct icon
						drawIconDirect(
							p0,
							p1_o,
							ImGui::GetColorU32(th.getColor<ThemeCol_Contact::avatar_online_direct>()),
							col_back
						);
					} else if (cstate->state == Contact::Components::ConnectionState::cloud) { // cloud icon
						drawIconCloud(
							p0,
							p1_o,
							ImGui::GetColorU32(th.getColor<ThemeCol_Contact::avatar_online_cloud>()),
							col_back
						);
					}
					ImGui::Dummy(p1_o);
					ImGui::SameLine(0.f, same_line_spacing);
				}

				if (
					const auto* slt = c.try_get<Contact::Components::StatusText>();
					slt != nullptr &&
					!slt->text.empty() &&
					slt->first_line_length > 0 &&
					slt->first_line_length <= slt->text.size()
				) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
					ImGui::TextUnformatted(slt->text.c_str(), slt->text.c_str() + slt->first_line_length);
					ImGui::PopStyleColor();
				} else {
					ImGui::TextDisabled(" "); // or dummy?
				}
			}

			// line 3
			//if (line_height >= 3) {
			//	constexpr std::string_view test_text{"text"};
			//	ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), ImVec2{}, ImVec2{}, 1.f, 1.f, test_text.data(), test_text.data()+test_text.size(), nullptr);
			//}
		}
	}
	ImGui::EndGroup();

	ImGui::SetCursorPos(post_curser_pos);

	ImGui::EndGroup();
	return got_selected;
}

