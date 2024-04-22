#include "./contact_list.hpp"

#include <solanaceae/contact/components.hpp>

#include <imgui/imgui.h>
//#include <imgui/imgui_internal.h>

#include <array>

static void drawIconDirectLines(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col,
	const float thickness
) {
#define PLINE(x0, y0, x1, y1) \
		ImGui::GetWindowDrawList()->AddLine( \
			{p0.x + p1_o.x*(x0), p0.y + p1_o.y*(y0)}, \
			{p0.x + p1_o.x*(x1), p0.y + p1_o.y*(y1)}, \
			col, \
			thickness \
		);

		// arrow 1
		// (3,1) -> (9,7)
		PLINE(0.3f, 0.1f, 0.9f, 0.7f)
		// (9,7) -> (9,5)
		PLINE(0.9f, 0.7f, 0.9f, 0.5f)
		// (9,7) -> (7,7)
		PLINE(0.9f, 0.7f, 0.7f, 0.7f)

		// arrow 2
		// (7,9) -> (1,3)
		PLINE(0.7f, 0.9f, 0.1f, 0.3f)
		// (1,3) -> (3,3)
		PLINE(0.1f, 0.3f, 0.3f, 0.3f)
		// (1,3) -> (1,5)
		PLINE(0.1f, 0.3f, 0.1f, 0.5f)
#undef PLINE
}

static void drawIconDirect(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main,
	const ImU32 col_back
) {
	// dark background
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	drawIconDirectLines(p0, p1_o, col_back, 4.0f);
	drawIconDirectLines(p0, p1_o, col_main, 1.5f);
}

static void drawIconCloud(
	const ImVec2 p0,
	const ImVec2 p1_o,
	const ImU32 col_main,
	const ImU32 col_back
) {
	std::array<ImVec2, 19> points {{
		{0.2f, 0.9f},
		{0.8f, 0.9f},
		{0.9f, 0.8f},
		{0.9f, 0.7f},
		{0.7f, 0.7f},
		{0.9f, 0.5f},
		{0.9f, 0.4f},
		{0.8f, 0.2f},
		{0.6f, 0.2f},
		{0.5f, 0.3f},
		{0.5f, 0.5f},
		{0.4f, 0.4f},
		{0.3f, 0.4f},
		{0.2f, 0.5f},
		{0.2f, 0.6f},
		{0.3f, 0.7f},
		{0.1f, 0.7f},
		{0.1f, 0.8f},
		{0.2f, 0.9f},
	}};
	for (auto& v : points) {
		v.y -= 0.1f;
		v = {p0.x + p1_o.x*v.x, p0.y + p1_o.y*v.y};
	}
	// the circle looks bad in light mode
	//ImGui::GetWindowDrawList()->AddCircleFilled({p0.x + p1_o.x*0.5f, p0.y + p1_o.y*0.5f}, p1_o.x*0.5f, col_back);
	ImGui::GetWindowDrawList()->AddPolyline(points.data(), points.size(), col_back, ImDrawFlags_None, 4.f);
	ImGui::GetWindowDrawList()->AddPolyline(points.data(), points.size(), col_main, ImDrawFlags_None, 1.5f);
}

void renderAvatar(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	ImVec2 box
) {
	ImVec4 color_current = th.getColor<ThemeCol_Contact::avatar_offline>();
	if (c.all_of<Contact::Components::ConnectionState>()) {
		const auto c_state = c.get<Contact::Components::ConnectionState>().state;
		if (c_state == Contact::Components::ConnectionState::State::direct) {
			color_current = th.getColor<ThemeCol_Contact::avatar_online_direct>();
		} else if (c_state == Contact::Components::ConnectionState::State::cloud) {
			color_current = th.getColor<ThemeCol_Contact::avatar_online_cloud>();
		}
	}

	// icon pos
	auto p0 = ImGui::GetCursorScreenPos();
	p0.x += box.x * 0.5f;
	p0.y += box.y * 0.5f;
	auto p1_o = box;
	p1_o.x *= 0.5f;
	p1_o.y *= 0.5f;

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

bool renderContactBig(
	const Theme& th,
	ContactTextureCache& contact_tc,
	const Contact3Handle c,
	int line_height,
	const bool unread,
	const bool selectable,
	const bool selected
) {
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

	ImGui::SameLine();
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
				ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x*0.5f);
			}

			ImGui::Text("%s%s", unread?"* ":"", (c.all_of<Contact::Components::Name>() ? c.get<Contact::Components::Name>().name.c_str() : "<unk>"));
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
					ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x*0.5f);
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
					ImGui::TextDisabled(""); // or dummy?
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

	return got_selected;
}

