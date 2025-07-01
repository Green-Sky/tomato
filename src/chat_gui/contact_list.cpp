#include "./contact_list.hpp"

#include <solanaceae/contact/components.hpp>
#include <solanaceae/message3/components.hpp>
#include <solanaceae/util/utils.hpp>
#include <solanaceae/util/time.hpp>

#include <imgui.h>
//#include <imgui_internal.h>

#include <entt/entity/runtime_view.hpp>

#include "./icons/direct.hpp"
#include "./icons/cloud.hpp"
#include "./icons/mail.hpp"
#include "./icons/person.hpp"
#include "./icons/group.hpp"

#include <cinttypes>

void renderAvatar(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const ContactHandle4 c,
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
	const ContactHandle4 c,
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
		if (c.all_of<Contact::Components::ID>()) {
			const auto id_str = bin2hex(c.get<Contact::Components::ID>().data);
			ImGui::Text("ID: %s", id_str.c_str());
		}

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

		// TODO: better time formatter
		const int64_t ts_now = getTimeMS();

		if (const auto* cfirst = c.try_get<Contact::Components::FirstSeen>(); cfirst != nullptr) {
			ImGui::Text("First seen: %" PRId64 "s ago", (ts_now - int64_t(cfirst->ts))/1000);
		}

		// TODO: fill with useful values periodically, maybe only show if not online?
		//if (const auto* clast = c.try_get<Contact::Components::LastSeen>(); clast != nullptr) {
		//    ImGui::Text("Last seen: %" PRId64 "s ago", (ts_now - int64_t(clast->ts))/1000);
		//}

		if (const auto* clasta = c.try_get<Contact::Components::LastActivity>(); clasta != nullptr) {
			ImGui::Text("Last Activity: %" PRId64 "s ago", (ts_now - int64_t(clasta->ts))/1000);
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
		const bool is_group = c.all_of<Contact::Components::TagGroup>();
		const bool is_private = c.all_of<Contact::Components::TagPrivate>();

		{ // line 1
			if (line_height == 1 && cstate != nullptr) {
				// icon pos
				auto p0 = ImGui::GetCursorScreenPos();
				p0.y += ImGui::GetStyle().FramePadding.y;
				ImVec2 p1_o = {img_y, img_y}; // img_y is 1 line_height in this case

				ImGui::Dummy(p1_o);
				if (ImGui::IsItemVisible()) {
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
				}
				ImGui::SameLine(0.f, same_line_spacing);
			}

			// we dont render group/private in 1lh mode
			if (line_height != 1 && (is_private || is_group)) {
				// icon pos
				auto p0 = ImGui::GetCursorScreenPos();
				p0.y += ImGui::GetStyle().FramePadding.y;
				const float box_hight = TEXT_BASE_HEIGHT - ImGui::GetStyle().FramePadding.y*2;
				ImVec2 p1_o = {box_hight, box_hight};

				ImGui::Dummy(p1_o);
				if (ImGui::IsItemVisible()) {
					if (is_private) {
						drawIconPerson(
							p0,
							p1_o,
							ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text))
						);
					} else if (is_group) {
						drawIconGroup(
							p0,
							p1_o,
							ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text))
						);
					}
				}
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

					ImGui::Dummy(p1_o);
					if (ImGui::IsItemVisible()) {
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
					}
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

	ImGui::EndGroup();
	return got_selected;
}

bool renderContactList(
	ContactRegistry4& cr,
	RegistryMessageModelI& rmm,
	const Theme& th,
	ContactTextureCache& contact_tc,
	const contact_const_runtime_view& view,

	// in/out
	ContactHandle4& selected_c
) {
	bool selection_changed {false};
	for (const Contact4 cv : view) {
		ContactHandle4 c{cr, cv};
		const bool selected = selected_c == c;

		// TODO: is there a better way?
		// maybe cache mm?
		bool has_unread = false;
		if (const auto* mm = rmm.get(c); mm != nullptr) {
			if (const auto* unread_storage = mm->storage<Message::Components::TagUnread>(); unread_storage != nullptr && !unread_storage->empty()) {
				has_unread = true;
			}
		}

		// TODO: expose line_height
		if (renderContactBig(th, contact_tc, c, 2, has_unread, true, selected)) {
			selected_c = c;
			selection_changed = true;
		}
	}
	return selection_changed;
}

